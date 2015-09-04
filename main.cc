#include <iostream>
#include <SDL2/SDL.h>

#include "utility.hh"
#include "testcomp.hh"
#include "testeff.hh"

using namespace std;

int main()
{
    TestComp tc;
    TestEff te;
    te.AddSource(0, &tc);
    te.AddSource(0, &tc);

    for (int i=0; i<100; ++i)
    {
	DrawSample(-1.0f, te.GetSample(), 1.0f, 70);
	te.NextSample();
    }
    te.Reset();
    cout << "------------------------------------" << endl;
    te.RemoveSource(0, &tc);
 
    for (int i=0; i<100; ++i)
    {
	DrawSample(-1.0f, te.GetSample(), 1.0f, 70);
	te.NextSample();
    }
    
    cout << "------------------------------------" << endl;
    for (int i=0; i<100; ++i)
    {
	DrawSample(-1.0f, tc.GetSample(), 1.0f, 70);
	tc.NextSample();
    }

    return 0;
}
