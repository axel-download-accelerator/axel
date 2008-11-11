
#include "tests.h"

int main(int argc, char** argv) {
	if (CU_initialize_registry() != CUE_SUCCESS) {
		return CU_get_error();
	}
	
	// Add new test suites here
	test_helper_suite();
	test_url_suite();
	
	
	CU_basic_set_mode(CU_BRM_VERBOSE);
	CU_basic_run_tests();
	
	int failure_count = CU_get_number_of_failures();
	
	CU_cleanup_registry();
	int cuerr = CU_get_error();
	
	return (cuerr != 0) ? cuerr : (failure_count > 0);
}
