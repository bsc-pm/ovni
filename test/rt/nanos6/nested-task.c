int main(void)
{
	#pragma oss task
	{
        #pragma oss task
        {
        }
        #pragma oss taskwait
	}
	return 0;
}
