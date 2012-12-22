/*
 * Copyright 2000-2002, Johan Nilsson. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "Prefs.h"

#include <File.h>
#include <FindDirectory.h>


Prefs::Prefs(const char* filename)
	:
	BMessage('pref')
{
	BFile file;

	fStatus = find_directory(B_USER_SETTINGS_DIRECTORY, &fPath);
	if (fStatus != B_OK)
		return;

	fPath.Append(filename);
	fStatus = file.SetTo(fPath.Path(), B_READ_ONLY);
	if (fStatus == B_OK)
		fStatus = Unflatten(&file);
}


Prefs::~Prefs()
{
	BFile file;

	if (file.SetTo(fPath.Path(), B_WRITE_ONLY | B_CREATE_FILE) == B_OK)
		Flatten(&file);
}


status_t Prefs::SetRef(const char* name, const entry_ref* ref)
{
	if (HasRef(name))
		return ReplaceRef(name, 0, ref);

	return AddRef(name, ref);

}


status_t Prefs::SetBool(const char* name, bool b)
{
	if (HasBool(name))
		return ReplaceBool(name, 0, b);

	return AddBool(name, b);
}


status_t Prefs::SetInt8(const char* name, int8 i)
{
	if (HasInt8(name))
		return ReplaceInt8(name, 0, i);

	return AddInt8(name, i);
}


status_t Prefs::SetInt16(const char* name, int16 i)
{
	if (HasInt16(name))
		return ReplaceInt16(name, 0, i);

	return AddInt16(name, i);
}


status_t Prefs::SetInt32(const char* name, int32 i)
{
	if (HasInt32(name))
		return ReplaceInt32(name, 0, i);

	return AddInt32(name, i);
}


status_t Prefs::SetInt64(const char* name, int64 i)
{
	if (HasInt64(name))
		return ReplaceInt64(name, 0, i);

	return AddInt64(name, i);
}


status_t Prefs::SetFloat(const char* name, float f)
{
	if (HasFloat(name))
		return ReplaceFloat(name, 0, f);

	return AddFloat(name, f);
}


status_t Prefs::SetDouble(const char* name, double f)
{
	if (HasDouble(name))
		return ReplaceDouble(name, 0, f);

	return AddDouble(name, f);
}


status_t Prefs::SetString(const char* name, const char* s)
{
	if (HasString(name))
		return ReplaceString(name, 0, s);

	return AddString(name, s);
}


status_t Prefs::SetPoint(const char* name, BPoint p)
{
	if (HasPoint(name))
		return ReplacePoint(name, 0, p);

	return AddPoint(name, p);
}


status_t Prefs::SetRect(const char* name, BRect r)
{
	if (HasRect(name))
		return ReplaceRect(name, 0, r);

	return AddRect(name, r);
}


status_t Prefs::SetMessage(const char* name, const BMessage* message)
{
	if (HasMessage(name))
		return ReplaceMessage(name, 0, message);

	return AddMessage(name, message);
}


status_t Prefs::SetFlat(const char* name, const BFlattenable* obj)
{
	if (HasFlat(name, obj))
		return ReplaceFlat(name, 0, (BFlattenable*) obj);

	return AddFlat(name, (BFlattenable*) obj);
}
