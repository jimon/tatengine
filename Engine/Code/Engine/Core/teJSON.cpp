/*
 *  teJSON.cpp
 *  TatEngine
 *
 *  Created by Dmitrii Ivanov on 08/2/11.
 *  Copyright 2011 Tatem Games. All rights reserved.
 *
 */

#include "teJSON.h"
#include "teLogManager.h"
#include "jsmn.h"

#if defined(WIN32) && !defined(__MINGW32__)
#include <stdlib.h>
#define strtoll _strtoi64
#define strtoull _strtoui64
#endif

namespace te
{
	namespace core
	{
		teJSONValue teJSONPool::GetRoot() {return teJSONValue(this, 0);}

		teJSONPool * ParseJSON(const teString & text, c8 * nestedBuffer, u32 nestedBufferSize)
		{
			const u32 tokensTempCount = 64;
			jsmntok_t tokensTemp[tokensTempCount];

			jsmn_parser parser;
			jsmn_init_parser(&parser, text.c_str(), tokensTemp, tokensTempCount);

			u32 totalTokensCount = 0;
			u32 totalDataStringsSize = 0;
			u32 totalDataNumbersSize = 0;

			jsmnerr_t error = JSMN_ERROR_NOMEM;

			while(error == JSMN_ERROR_NOMEM)
			{
				parser.curtoken = 0;
				parser.cursize = NULL;
				for(u32 i = 0; i < tokensTempCount; ++i)
				{
					if(tokensTemp[i].start != -1)
					{
						if(tokensTemp[i].type == JSMN_STRING)
							totalDataStringsSize += teUTF8GetSizeWithReplacedEscapeCharacters(text.c_str() + tokensTemp[i].start, tokensTemp[i].end - tokensTemp[i].start + 1);
						else if(tokensTemp[i].type == JSMN_PRIMITIVE)
						{
							c8 primitiveChar = text.c_str()[tokensTemp[i].start];
							if((primitiveChar != 't') && (primitiveChar != 'f') && (primitiveChar != 'n'))
								totalDataNumbersSize += sizeof(f64) + sizeof(s64) + sizeof(u64); // size for number
						}
						++totalTokensCount;
					}
					tokensTemp[i].type = JSMN_PRIMITIVE; tokensTemp[i].start = -1; tokensTemp[i].end = -1; tokensTemp[i].size = 0;
				}

				error = jsmn_parse(&parser);
			}

			if(error != JSMN_SUCCESS)
				return NULL;

			for(u32 i = 0; i < tokensTempCount; ++i)
			{
				if(tokensTemp[i].start != -1)
				{
					if(tokensTemp[i].type == JSMN_STRING)
						totalDataStringsSize += teUTF8GetSizeWithReplacedEscapeCharacters(text.c_str() + tokensTemp[i].start, tokensTemp[i].end - tokensTemp[i].start + 1);
					else if(tokensTemp[i].type == JSMN_PRIMITIVE)
					{
						c8 primitiveChar = text.c_str()[tokensTemp[i].start];
						if((primitiveChar != 't') && (primitiveChar != 'f') && (primitiveChar != 'n'))
							totalDataNumbersSize += sizeof(f64) + sizeof(s64) + sizeof(u64); // size for number
					}
					++totalTokensCount;
				}
			}

			jsmntok_t * tokens = (jsmntok_t*)TE_ALLOCATE(sizeof(jsmntok_t) * totalTokensCount);
			jsmn_init_parser(&parser, text.c_str(), tokens, totalTokensCount);

			error = jsmn_parse(&parser);

			if(error != JSMN_SUCCESS)
				return NULL;

			u8 numbersAlignment = sizeof(f32);
			u32 totalDataSize = totalDataStringsSize + totalDataNumbersSize + numbersAlignment;

			u32 poolNeededSize = sizeof(teJSONPool) + sizeof(teJSONToken) * totalTokensCount + totalDataSize;
			teJSONPool * json = NULL;

			if(nestedBuffer && nestedBufferSize)
			{
				if(nestedBufferSize >= poolNeededSize)
					json = (teJSONPool*)nestedBuffer;
				else
					return NULL;
			}
			else
				json = (teJSONPool*)TE_ALLOCATE(poolNeededSize);

			json->dataSize = totalDataSize;
			json->tokensCount = totalTokensCount;

			teJSONValue cursor = json->GetRoot();

			if(json->tokensCount)
				json->tokens[0] = teJSONToken();

			u32 dataStringPosition = 0;
			uintptr_t ptrFloatBase = (uintptr_t)json->GetData(totalDataStringsSize);
			u32 dataFloatPosition = totalDataStringsSize + TE_ALIGN_PTR(ptrFloatBase, numbersAlignment) - ptrFloatBase;
			u32 freePosition = 0;

			for(u32 i = 0; i < totalTokensCount; ++i)
			{
				switch(tokens[i].type)
				{
				case JSMN_PRIMITIVE:
					{
						c8 primitiveChar = text.c_str()[tokens[i].start];

						if(primitiveChar == 't') cursor.Get().type = teJSONToken::VT_TRUE;
						else if(primitiveChar == 'f') cursor.Get().type = teJSONToken::VT_FALSE;
						else if(primitiveChar == 'n') cursor.Get().type = teJSONToken::VT_NULL;
						else
						{
							cursor.Get().type = teJSONToken::VT_NUMBER;
							cursor.Get().dataOffset = dataFloatPosition;
							f64 * number1 = reinterpret_cast<f64*>(json->GetData(dataFloatPosition));
							s64 * number2 = reinterpret_cast<s64*>(json->GetData(dataFloatPosition + sizeof(f64)));
							u64 * number3 = reinterpret_cast<u64*>(json->GetData(dataFloatPosition + sizeof(f64) + sizeof(s64)));
							*number1 = (f64)atof(text.c_str() + tokens[i].start);
							*number2 = (s64)strtoll(text.c_str() + tokens[i].start, NULL, 0);
							*number3 = (u64)strtoull(text.c_str() + tokens[i].start, NULL, 0);

							dataFloatPosition += sizeof(f64) + sizeof(s64) + sizeof(u64);
						}
						cursor = cursor.Next();

						break;
					}
				case JSMN_STRING:
					{
						cursor.Get().type = teJSONToken::VT_STRING;
						cursor.Get().dataOffset = dataStringPosition;
						teUTF8StrCpyReplaceEscapeCharacters(json->GetData(dataStringPosition), text.c_str() + tokens[i].start, tokens[i].end - tokens[i].start);
						*json->GetData(dataStringPosition + tokens[i].end - tokens[i].start) = '\0';
						dataStringPosition += tokens[i].end - tokens[i].start + 1;
						cursor = cursor.Next();
						break;
					}
				case JSMN_OBJECT:
				case JSMN_ARRAY:
					{
						cursor.Get().type = (tokens[i].type == JSMN_OBJECT) ? teJSONToken::VT_OBJECT : teJSONToken::VT_ARRAY;

						if(tokens[i].size)
						{
							cursor.Get().childrensBegin = freePosition + 1;
							cursor.Get().childrensEnd = freePosition + tokens[i].size;

							freePosition += tokens[i].size;

							for(u32 j = cursor.Get().childrensBegin; j <= cursor.Get().childrensEnd; ++j)
							{
								json->Get(j).type = teJSONToken::VT_INVALID;
								json->Get(j).parentIndex = cursor.position;
							}
						}
						else
						{
							cursor.Get().childrensBegin = u32Max;
							cursor.Get().childrensEnd = u32Max;
						}

						cursor = cursor.Next();

						break;
					}
				default:
					TE_ASSERT(0);
					break;
				}
			}

			TE_FREE(tokens);

			return json;
		}

		teJSONPool * ParseJSON(core::IBuffer * buffer, c8 * nestedBuffer, u32 nestedBufferSize)
		{
			if(!buffer)
				return NULL;

			if(buffer->GetArray())
				return ParseJSON(teString((const c8*)buffer->GetArray()), nestedBuffer, nestedBufferSize);
			else
			{
				c8 * text = (c8*)TE_ALLOCATE(buffer->GetSize() + 1);

				buffer->Lock(core::BLT_READ);
				buffer->SetPosition(0);
				buffer->Read(text, buffer->GetSize());
				buffer->Unlock();

				text[buffer->GetSize()] = '\0';

				teJSONPool * result = ParseJSON(teString(text), nestedBuffer, nestedBufferSize);

				TE_FREE(text);

				return result;
			}
		}
	}
}
