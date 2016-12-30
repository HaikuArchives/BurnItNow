/*
 * Copyright 2016. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *	Humdinger, humdingerb@gmail.com
 */

#ifndef APPSETTINGS_H
#define APPSETTINGS_H

#include <Locker.h>
#include <Rect.h>

class AppSettings {
public:
					AppSettings();
					~AppSettings();

		bool		Lock();
		void		Unlock();
			
		bool		GetEject();
		bool		GetCache();
		int32		GetSpeed();
		BRect		GetWindowPosition();
		void		GetSplitWeight(float& left, float& right);
		void		GetSplitCollapse(bool& left, bool& right);

		void		SetEject(bool eject);
		void		SetCache(bool cache);
		void		SetSpeed(int32 speed);
		void		SetWindowPosition(BRect where);
		void		SetSplitWeight(float left, float right);
		void		SetSplitCollapse(bool left, bool right);
private:
		bool		fEject;
		bool		fCache;
		int32		fSpeed;
		BRect		fPosition;
		float		fInfoWeight;
		float		fTracksWeight;
		bool		fInfoCollapse;
		bool		fTracksCollapse;

		bool		dirtySettings;
		
		BLocker		fLock;
};

#endif	/* APPSETTINGS_H */
