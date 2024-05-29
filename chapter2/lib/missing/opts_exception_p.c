#include <ruby.h>

int
opts_exception_p(VALUE opts)
{
    static ID kwds[1];
    VALUE exception;
    if (!kwds[0]) {
        kwds[0] = rb_intern_const("exception");
    }
    if (!rb_get_kwargs(opts, kwds, 0, 1, &exception)) return 1;
    switch (exception) {
      case Qtrue: case Qfalse:
        break;
      default:
        rb_raise(rb_eArgError, "true or false is expected as exception: %+"PRIsVALUE,
                 exception);
    }
    return exception != Qfalse;
}
