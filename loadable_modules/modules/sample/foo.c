
int bbb;
//int bbb2 = 5;

int sample_Placeholder[256] __attribute__((section(".first")));

int foo()
{
sample_Placeholder[0]++;
    return 555;
}

__attribute__((section(".module.sample")))
const struct blah {
    int a;
    int b;
} blah = {111,222};

__attribute__((section(".cmi.text"))) int sample_PackageEntryPoint()
{
    return foo();
}
