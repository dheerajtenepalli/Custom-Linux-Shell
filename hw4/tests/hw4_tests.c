#include <criterion/criterion.h>
#include <criterion/logging.h>

#include "jobber.h"

Test(jobber_tests_suite, startup_test) {
    char *cmd = "jobber < /dev/null";
    int return_code = WEXITSTATUS(system(cmd));

    cr_assert_eq(return_code, EXIT_SUCCESS,
                 "Program exited with %d instead of EXIT_SUCCESS",
		 return_code);
}
