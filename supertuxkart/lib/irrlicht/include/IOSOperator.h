// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __I_OS_OPERATOR_H_INCLUDED__
#define __I_OS_OPERATOR_H_INCLUDED__

#include "IReferenceCounted.h"
#include "irrString.h"

namespace irr
{

//! The Operating system operator provides operation system specific methods and informations.
class IOSOperator : public virtual IReferenceCounted
{
public:
	//! Get the current operation system version as string.
	virtual const core::stringc& getOperatingSystemVersion() const = 0;

	//! Get the current operation system version as string.
	/** \deprecated Use getOperatingSystemVersion instead. This method will be removed in Irrlicht 1.9. */
	_IRR_DEPRECATED_ const wchar_t* getOperationSystemVersion() const
	{
		return core::stringw(getOperatingSystemVersion()).c_str();
	}

	//! Copies text to the clipboard
#if defined(_IRR_COMPILE_WITH_WINDOWS_DEVICE_)
	virtual void copyToClipboard(const wchar_t* text) const = 0;
#else
	virtual void copyToClipboard(const c8* text) const = 0;
#endif

	//! Get text from the clipboard
	/** \return Returns 0 if no string is in there. */
#if defined(_IRR_COMPILE_WITH_WINDOWS_DEVICE_)
	virtual const wchar_t* getTextFromClipboard() const = 0;
#else
	virtual const c8* getTextFromClipboard() const = 0;
#endif
};

} // end namespace

#endif
