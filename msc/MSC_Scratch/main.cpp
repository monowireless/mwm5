#include <cstdint>

#include <filesystem>
#include <iostream>
namespace fs = std::filesystem;
#include <algorithm>
#include <string>


#include <windows.h>
#include <debugapi.h>
#include "printf/printf.h"

#include <utility>
#include <type_traits>
#include "twecommon.h"

extern int main_app();
extern int main_scratch();

#include "mwm5.h"

#include <iostream>
#include <regex>
#include <string>

void print_str(const wchar_t* ptr) {
	wprintf(L"file=%s\n", ptr);
}

struct B {
	virtual void run() {}
};
struct A0 : public B {
	void run() {
		std::cout << "A0" << std::endl;
	}
};

struct A1 : public B{
	void run() {
		std::cout << "A1" << std::endl;
	}
};

A0 objA0;
A1 objA1;
B& refA = objA0;

void scratch() {
	refA.run();
	refA = objA1;
	refA.run();
}

void scratch_template_enableif() {
	std::is_integral<const wchar_t>::value;

	typedef const wchar_t*& T;
	std::is_reference<T>::value;
	std::is_pointer<T>::value;

	// check is T is pointer type, with any reference, with/without const.
	static_assert(
		std::is_pointer<typename std::remove_reference<T>::type>::value &&
		std::is_same<wchar_t,
			typename std::remove_cv<
				typename std::remove_pointer<
					typename std::remove_reference<T>::type
				>::type
			>::type
		>::value
		, "NG");

	typedef const wchar_t (&S)[10];
	std::is_reference<S>::value;
	std::is_pointer<S>::value;
	std::is_array<typename std::remove_reference<S>::type>::value;

	// check is S is array type, with any reference, with/without const.
	static_assert(
		std::is_array<typename std::remove_reference<S>::type>::value &&
		std::is_same<wchar_t,
			typename std::remove_const<
				typename std::remove_all_extents<
					typename std::remove_reference<S>::type
				>::type
			>::type
		>::value
		, "NG");
}

void scratch_makefp() {
	SmplBuf_WChar a = L"ABCD";
	SmplBuf_WChar b = a.c_str();

	print_str(make_full_path(L"ABC").c_str());
	print_str(make_full_path((wchar_t*)L"DEF").c_str());
	print_str(make_full_path(SmplBuf_WChar(L"FGH")).c_str());

	print_str(make_full_path(L"TEST").c_str());
	print_str(make_full_path(SmplBuf_WChar(L"HELLO!") + L"WORLD", L"", SmplBuf_WChar(), make_file_ext(L"TEST", L"SAV")).c_str());

	
	exit(0);
}

void scratch_sort() {
	SimpleBuffer<SmplBuf_WChar> ary1;
	SimpleBuffer<int> ary2;

	ary1.push_back(SmplBuf_WChar(L"HELLO!"));
	ary2.push_back(1);

	ary1.push_back(SmplBuf_WChar(L"foo!"));
	ary2.push_back(2);

	ary1.push_back(SmplBuf_WChar(L"bar!"));
	ary2.push_back(3);

	ary1.push_back(SmplBuf_WChar(L"brabo!"));
	ary2.push_back(4);

	SmplBuf_Sort(ary1[0]);

	ary1.push_back(SmplBuf_WChar(L"charie!"));
	ary2.push_back(5);

	//SmplBuf_Sort2_NoCase(ary1, ary2);
	SmplBufStrA_Sort2_NoCase(ary1, ary2);

	for (int i = 0; i < int(ary1.size()); i++) {
		wprintf(L"%s=%d\n", ary1[i].c_str(), ary2[i]);
	}

	SmplBuf_Sort(ary2);
	for (int i = 0; i < int(ary2.size()); i++) {
		wprintf(L"%d", ary2[i]);
	}

	return;
}

#if 1
void scratch_wchar_string() {
	wchar_t a[] = { L'1', 0, 0 };
	const wchar_t* pa = a;
	SmplBuf_WChar work = make_full_path(L"a", pa, L"c"); // TODO: cause an error (does not support const wchar_t*)
	//SmplBuf_WChar f = make_file_ext(L"test", L"save");
	// SmplBuf_WChar work2 = make_full_path(work, f);

#if 0
	work2.emptify(100) << L"HELLO WORLD! TEST2";
	work2.resize_preserving_unused(13); // should be !
	work2.c_str(); // mark terminator

	work2.pop_back();
	work2.c_str(); // mark terminator

	work2.emptify();
	work2.c_str(); // mark terminator
	work2.pop_back();

	work2 << L"TEST3"; work2.c_str();
	work2 = L"HELLO3"; work2.c_str();
	wchar_t b[] = L"CLAIR!", *p_b = b; 
	work2 = b;  work2.c_str();
	work2 = as_copying(p_b); work2.c_str();
	SmplBuf_WChar work3 = make_full_path(work, make_file_ext(L"test", L"save")); // can't do this so far... (give up...)
	
	//SmplBuf_WChar work3 = work2 + (work2 + L"HELLO");

	work3 = make_full_path(work3, (const wchar_t*)L"AAA", work2);
#endif
	return;
}
#endif

void scratchX() {
	if (std::regex_match("subject", std::regex("(sub)(.*)")))
		std::cout << "string literal matched\n";

	const char cstr[] = "subject";
	std::string s("subject");
	std::regex e("(sub)(.*)");

	if (std::regex_match(s, e))
		std::cout << "string object matched\n";

	if (std::regex_match(s.begin(), s.end(), e))
		std::cout << "range matched\n";

	std::cmatch cm;
	std::regex_match(cstr, cm, e);
	std::cout << "string literal with " << cm.size() << " matches\n";

	std::smatch sm;
	std::regex_match(s, sm, e);
	std::cout << "string object with " << sm.size() << " matches\n";

	std::regex_match(s.cbegin(), s.cend(), sm, e);
	std::cout << "range with " << sm.size() << " matches\n";


	std::regex_match(cstr, cm, e, std::regex_constants::match_default);

	std::cout << "the matches were: ";
	for (unsigned i = 0; i < sm.size(); ++i) {
		std::cout << "[" << sm[i] << "] ";
	}

	std::cout << std::endl;
}
void scratchY() {
	char buff[1024];
	char buff_re[1024];

	std::regex re("(sub)(.*)");
	std::cmatch m;
	std::regex_match("subject", m, re);
	std::cout << "->" << m.size() << ":" << m.str() << std::endl;

	while (fgets(buff, 1024, stdin)) {
		if (buff[0] == ':') {
			int i = 1, j = 0;
			for (; i < sizeof(buff); i++) {
				if (buff[i] == 0) break;
				if (buff[i] >= 0x20 && buff[i] <= 0x7F) {
					buff_re[j++] = buff[i];
				}
			}
			buff_re[j] = 0;
			printf("regex->\"%s\"\r\n", buff_re);
			re = std::regex(buff_re);
		}
		else {
			std::cmatch m;
			auto result = std::regex_search(buff, m, re);

			std::cout << '[' << result << ']';
			for (unsigned i = 0; i < m.size(); i++) {
				// m[0]: whole match string m[1]:sub1, m[2]:sub2
				std::cout << i << ':' << m[i].str();
			}
			std::cout << std::endl;
		}
	}
}

void scratch0() {
	SmplBuf_Byte b(100), b2(100);

	b.push_back(1);
	b.push_back(2);
	b.push_back(5);
	b.push_back(3);
	b.push_back(6);

	auto&& rb = backwards(b); // b;
	auto it = std::find_if(rb.begin(), rb.end()
		, [](auto x) { return x == 3 || x == 5; }
	);

	printf("\r\nfound = %d", *it);
#if 1
	if (it != rb.end()) {
		printf("\r\npos = %d, rpos = %d"
			, it - rb.begin()
			, rb.end() - it - 1
		);
	}
#endif

	b2 << std::pair(b.rbegin(), b.rend());
}

int main() {
	scratch();

	// the OS dependent initialize
	TWESYS::SysInit();

	printf("start MSC test code. choose one:\n");
	printf(" 1: interactive,  2: scratch test codes -> ");

	int c = getchar();
	switch (c) {
	case '1': main_app(); break;
	case '2': main_scratch(); break;
	default: 
		printf("not chosen, enter to exit...");
		(void)getchar();
	}

	return 0;
}
