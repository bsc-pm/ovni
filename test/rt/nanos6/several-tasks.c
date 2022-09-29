int main(void)
{
	for(int i = 0; i < 5000; i++)
	{
		#pragma oss task
		{
			for(volatile long j = 0; j < 10000L; j++)
			{
			}
		}
	}
	#pragma oss taskwait
	return 0;
}
