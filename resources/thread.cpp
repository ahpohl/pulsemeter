#include <iostream>
#include <thread>
 
void thread_function(void)
{
	int i;

    for (i = 0; i < 10; i++)
	{
        std::cout << "Thread function Executing" << std::endl;
	}
}
 
int main(int argc, char * argv[])  
{
	int i;    

    std::thread threadObj(thread_function);
    
	for (i = 0; i < 10; i++)
	{
        std::cout << "Display From MainThread" << std::endl;
	}    

	threadObj.join();    
    std::cout << "Exit of Main function" << std::endl;

    return 0;
}
