
#include <puppy_core.h>

static void say_hello(void) {
    printk("hello! puppy-rtos!\n");
}
PUP_INIT_FUNC(say_hello);

void *puppy_main_thread(void *arg){
    // while(1){
    //     printk("main thread\n");
    //     pup_delay(1000);
    // }
    return NULL;
}

int main( void ) {
    puppy_board_init();
    puppy_init();
	return 0;
}
