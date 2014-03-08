
#include <interface/vcos/vcos.h>

void run_logging_tests(int *run, int *passed)
{
   VCOS_LOG_CAT_T cat0;

   /* check we can register twice and not lock up */
   vcos_log_register("cat", &cat0);
   vcos_log_register("cat", &cat0);

   vcos_log_unregister(&cat0);
   vcos_log_unregister(&cat0);

   (*run)++;
   (*passed)++;

}

