
int ccc;
//int ccc2 = 7;

int sample2_Placeholder[256] __attribute__((section(".first")));

static int boo()
{
sample2_Placeholder[0]++;
    return 7777;
}

__attribute__((section(".cmi.text"))) int sample2_PackageEntryPoint()
{
    return boo();
}
