#include "malloc.h"

int main()
{
	test_split_block();
}

void test_split_block()
{
	block_meta *block = request_space(NULL, 40);
	split_block(5, block);
}