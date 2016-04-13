static void	start_measruing()
{
		//measuredTWIz = read_reg(READ_Z_AXIS, 0x00); 
		//*str = itoa(measuredTWIz, 10);
		

				
		//int readread = read_reg(READ_Z_AXIS, 0x00);
		//numberOfMeasurements++;
	
		char stringa[150]; 
	  //const char anorhterString[100];
		//char *stringa = malloc( sizeof(char) * ( 100 + 1 ) );
		//char * const anotherString = malloc( sizeof(char) * ( 100 + 1) );
		//char aThird[30];
		int one = 37;
		int two = 210;
	
		//strcpy(stringa, intToChar(one));
		
		//strcpy(stringa, append("f", "q"));
		
		//char c = one + '0'; 
		//appendchar(stringa, 100, c);
		//appendchar(stringa, 100, ' ');
		
		//strcat(stringToSend, intToChar(one));
		
		sprintf(stringa, "%d ", two);
		
		for (int i = 0; i < 150; i++)
		{	
				if(stringa[i] == '\0')
				{
						break;
				}
				else
				{
				appendchar(stringToSend, 150, stringa[i]);
				}
		}
		


		// THIS WORKS when static char * stringToSend is defined on top;
		
		//char stringa[50];
		//const char *one = "We";
		//const char *two = "Are";
		//char *three = "Creating\0";
		//char *four = "a\0";
		//char *five = "String\0";
	
		//sprintf(stringa, "%s %s %s %s %s", one, two, three, four, five);

		//*stringToSend = stringa; 
		
		// TO HERE: THIS WORKS
		}
