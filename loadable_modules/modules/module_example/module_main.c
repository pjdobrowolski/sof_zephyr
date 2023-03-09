#include "../include/loadable_processing_module.h"

//struct _module_example{
//	int cos;
//	}module_example;

DECLARE_LOADABLE_MODULE(module_example)

int loadable_module_main(void){
	while(1);
	return 0;
}
