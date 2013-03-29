#pragma once



// -------------------------------------------------------------------------------
unsigned int	StringHash(const char *string)
{
	unsigned int hash = 2166136261;

	char *pchar = (char *)string;
	char lower;
	while (*pchar != NULL)
	{
		// lowercase only
		lower = *pchar;
		if (lower >= 'A' && lower <= 'Z')
			lower = lower - 'A' + 'a';

		hash = hash ^ lower;
		hash *= 16777619;
		pchar++;
	}

	return hash;
}
