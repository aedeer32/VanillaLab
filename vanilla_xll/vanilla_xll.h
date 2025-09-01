// The following ifdef block is the standard way of creating macros which make exporting
// from a DLL simpler. All files within this DLL are compiled with the VANILLAXLL_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see
// VANILLAXLL_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef VANILLAXLL_EXPORTS
#define VANILLAXLL_API __declspec(dllexport)
#else
#define VANILLAXLL_API __declspec(dllimport)
#endif

// This class is exported from the dll
class VANILLAXLL_API Cvanillaxll {
public:
	Cvanillaxll(void);
	// TODO: add your methods here.
};

extern VANILLAXLL_API int nvanillaxll;

VANILLAXLL_API int fnvanillaxll(void);
