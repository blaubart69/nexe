#include "stdafx.h"
#include "CppUnitTest.h"

#include "..\nexe\ProcessRedirect.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace TestNExe
{		
	TEST_CLASS(TestProcessRedirect)
	{
	public:
		
		TEST_METHOD(JustStartCmdExeWithNoParameters)
		{
			static bool ImFinished = false;
			ProcessRedirect pr;
			Assert::IsTrue(
				pr.Start(L"c:\\windows\\system32\\cmd.exe", L"/c dir c:", "localhost", [](const char* buf, size_t size, LPVOID context) {
					ImFinished = true;
				}, NULL)
			, L"something wrong to start the exe");

			/*Assert::AreEqual(WAIT_IO_COMPLETION, SleepEx(2000, TRUE));
			Assert::AreEqual(WAIT_IO_COMPLETION, SleepEx(2000, TRUE));*/
			while (SleepEx(1000, TRUE) == WAIT_IO_COMPLETION) {
				;
			}

			Assert::IsTrue(ImFinished, L"onCompleted was not called");
		}

	};
}