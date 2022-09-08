int main(void)
{
	for(int i = 0; i < 10000; i++)
	{
		#pragma oss task
		{
			for(volatile long j = 0; j < 1000000L; j++)
			{
			}
		}
	}
	#pragma oss taskwait
	return 0;
}
