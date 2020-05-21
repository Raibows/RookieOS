#include <string.h>

int tools_str2int(char* str, int s, int e) {
    /*
     * 开区间[s, e)
     */
    s = s > 0 ? s : 0;
    e = e > 0 ? e : strlen(str);
    int val = 0;
    for (; s < e; ++s)
    {
        if (str[s] >= '0' && str[s] <= '9')
        {
            val *= 10;
            val += (str[s] - '0');
        }
        else return -1;
    }
    return val;
}







