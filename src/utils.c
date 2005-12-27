#include <gtk/gtk.h>
#include <string.h>

gchar *
remove_whitespace(gchar *string)
{
	int i, j;
	gchar *buf;
	
	j = 0;
	buf = g_new(gchar, strlen(string)); // Possibly wasteful.
	
	for(i = 0; i < strlen(string); i++)
	{
		if((gchar)string[i] > (gchar)' ')
		{
			buf[j] = string[i];
			j++;
		}
	}
	
	buf[j] = '\0';
	
	return buf;
}
