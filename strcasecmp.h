/*
    strcasecmp.h -- part of texgenpack, a texture compressor using fgen.

    texgenpack -- a genetic algorithm texture compressor.
    Copyright 2013 Harm Hanemaaijer

    This file is part of texgenpack.

    texgenpack is free software: you can redistribute it and/or modify it
    under the terms of the GNU Lesser General Public License as published
    by the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    texgenpack is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
    License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with texgenpack.  If not, see <http://www.gnu.org/licenses/>.

*/


static void strtoupper(char *str) {
	int n = strlen(str);
	for (int i = 0; i < n; i++)
		str[i] = toupper(str[i]);
}

static int strcasecmp(const char *str1, const char *str2) {
	char *str1_upper = (char *)alloca(strlen(str1) + 1);
	strcpy(str1_upper, str1);
	strtoupper(str1_upper);
	char *str2_upper = (char *)alloca(strlen(str2) + 1);
	strcpy(str2_upper, str2);
	strtoupper(str2_upper);
	int r = strcmp(str1_upper, str2_upper);
	return r;
}

