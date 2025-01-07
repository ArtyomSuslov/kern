#include <inc/lib.h>

double a = 1.23456789;
double b = 1.23456789e-10;
double c = 1.23456789e+10;


void test_cprintf() {
    cprintf("\ncprintf test:\n");
    cprintf("a=%f b=%f c=%f\n", a, b, c);
    cprintf("a=%e b=%e c=%e\n", a, b, c);
    cprintf("a=%g b=%g c=%g\n", a, b, c);
}

void test_snprintf() {
    char buffer[256];

    snprintf(buffer, sizeof(buffer), "\nsnprintf test:\n");
    cprintf("%s", buffer);

    snprintf(buffer, sizeof(buffer), "a=%f b=%f c=%f\n", a, b, c);
    cprintf("%s", buffer);

    snprintf(buffer, sizeof(buffer), "a=%e b=%e c=%e\n", a, b, c);
    cprintf("%s", buffer);

    snprintf(buffer, sizeof(buffer), "a=%g b=%g c=%g\n", a, b, c);
    cprintf("%s", buffer);
}

void test_zero() {
    cprintf("\nzero test\n");
    cprintf("%f\n", 0.0);
    cprintf("%e\n", 0.0);

    cprintf("%g\n", 0.0);
    cprintf("%#g\n", 0.0);
}

void
test_inf()
{
    cprintf("\ninf test\n");
    cprintf("%f\n", 1.0 / 0.0);
    cprintf("%f\n", -1.0 / 0.0);
    cprintf("%F\n", 1.0 / 0.0);
}

void
test_nan()
{
    cprintf("\nnan test\n");
    cprintf("%f\n", 0.0 / 0.0);
    cprintf("%F\n", 0.0 / 0.0);
}

void
test_prec()
{
    cprintf("\nprecision test\n");
    cprintf("%.*f\n", 2, 1.234567);
    cprintf("%.2f\n", 1.234567);
    cprintf("%.2f\n", 1.56789);
    cprintf("%.6f\n", 1.2);
}


void
umain(int argc, char **argv) {
    cprintf("\n\nProgram to testing float operations:\n");

    test_cprintf();
    test_snprintf();
    test_zero();
    test_inf();
    test_nan();
    test_prec();

    cprintf("\n\n");
}
