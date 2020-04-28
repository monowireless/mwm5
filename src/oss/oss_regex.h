#ifndef REGEX_H
#define REGEX_H

#include "../twesettings/twecommon.h"

class oss_regex {
public:
	typedef unsigned char CHAR;

private:
	const int MAXNFA;
	static const int MAXTAG = 10;
	static const int MAXCHR = 128;
	static const int CHRBIT = 8;
	static const int BITBLK = MAXCHR / CHRBIT;

	int  tagstk[MAXTAG];     /* subpat tag stack..*/
	CHAR *nfa;				 /* automaton..       */
	int  sta;                /* status of lastpat */
	CHAR bittab[BITBLK];	 /* bit table for CCL */

	const char* bol;
	const char* bopat[MAXTAG];
	const char* eopat[MAXTAG];
	CHAR chrtyp[MAXCHR];

	CHAR* _mp; // working pointer of nfa

public:

	oss_regex(int maxnfa = 512)
		: MAXNFA(maxnfa)
		, tagstk{}
		, nfa{}
		, sta(0)
		, bittab{}
		, chrtyp{
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0, 1, 1,
			1, 1, 1, 1, 1, 1, 1, 1, 0, 0,
			0, 0, 0, 0, 0, 1, 1, 1, 1, 1,
			1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
			1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
			1, 0, 0, 0, 0, 1, 0, 1, 1, 1,
			1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
			1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
			1, 1, 1, 0, 0, 0, 0, 0}
		, bol(nullptr)
		, bopat{}
		, eopat{}
		, _mp(nullptr)
	{
		nfa = new CHAR[MAXNFA]{};
	}

	~oss_regex() {
		delete[] nfa;
	}

	char* comp(const char*);
	bool_t exec(const char*);
	void modw(const char*);
	bool_t subs(const char*, char*);

	int _get_nfa_size() {
		return (int)(_mp - nfa);
	}
private:

	const char* pmatch(const char*, CHAR*);
	inline void chset(CHAR c);
	inline void store(CHAR x);
};

#endif /* REGEX_H */
