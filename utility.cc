#include "utility.hh"

void DrawSample(float min, float value, float max, int charlimit)
{
    int counter = 0;
    float newValue = value - min;
    int charPosition = int(charlimit * newValue / (max - min));
    std::cout << min << "|";
    while (counter < charlimit)
    {
	if (counter != charPosition)
	{
	    std::cout << " ";
	}
	else
	{
	    std::cout << "#";
	}
	++counter;
    }
    std::cout << "|" << max << std::endl;
}
