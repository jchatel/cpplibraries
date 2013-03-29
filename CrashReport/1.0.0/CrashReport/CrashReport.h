#pragma once

namespace CrashReport {


	void __fastcall Initialize(const char *fileprefix);

	int __cdecl Create(PEXCEPTION_POINTERS pExceptPtrs);



}

