//
//  tePlatform_iOS.h
//  TatEngine
//
//  Created by Igor Leontiev on 05/13/11.
//  Copyright Tatem Games 2011. All rights reserved.
//

#ifndef TE_TEPLATFORM_IOS_H
#define TE_TEPLATFORM_IOS_H

#include "TatEngineCoreConfig.h"
#include "teApplicationManager.h"
#include "teString.h"
#include "teSignal.h"
#include "tePlatform.h"

namespace te
{
	namespace core
	{
		typedef void (*teGetUserInputCallback)(const te::c8 * data, te::teptr_t userData);
		
		//! Platform iPhone
		class tePlatform_iOS : public te::teReferenceCounter
		{
		public:
			//! Constructor
			tePlatform_iOS();
			
			//! Destructor
			~tePlatform_iOS();
			
			//! Get Device UDID
			const te::c8 * GetDeviceUDID();
			
			//! App Move From Active To Inactive
			teSignal OnWillResignActive;
			
			//! App Enter Background
			teSignal OnDidEnterBackground;
			
			//! Return From Background
			teSignal OnWillEnterForeground;
			
			//! Become Active
			teSignal OnBecomeActive;
			
			//! Will Terminate
			teSignal OnWillTerminate;
			
			//! Open Link In Safari
			void OpenLink(teString link);
			
			void SendMail(teString url);
			
			//! Show Chartboost More Apps Page
			void ShowChartbosstMoreApps();
			
			void DismissAllAlertView();
			
			EDeviceType DefinePlatform();
			
			void GetUserInputText(const teString & title, const teString & question, const teString & yesBtn, const teString & noBtn, teGetUserInputCallback callback, te::teptr_t userData);
			
			void AskUserQuestion(const teString & title, const teString & question, const teString & yesBtn, const teString & noBtn, teGetUserInputCallback callback, te::teptr_t userData);
			
			f32 GetBrightness();
			
			void PlayVideo(u8 index, void * callbackActor);
			
			void OnVideoFinished();
			
			void ReadUserPref(const c8 * name, c8 * returnArray, u32 returnArraySize, u32 * resultSize = NULL);
			
			const c8 * GetDeviceLocale();
			
		protected:
			void * videoCallbackActor;
		};
	}
}

#endif
