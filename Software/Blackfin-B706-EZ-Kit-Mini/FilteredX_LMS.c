/* filter the signal - audio processing */
delayInput[0] = rbits((short)((pRxBuffer[0])<<8));
delayError[0] = rbits((short)((pRxBuffer[1])<<8));

	/* Estimate channel filter */

			    // Convolution
			    sum = 0.0k;
			    for(i=0; i<NUM_COEFF; i++){
			        sum += delayInput[i]*plant1_coeffs[i];
			    } // Convolution loop

			    // shift delay line
			    for(j=NUM_COEFF-1;j>0;j--){
					    delayInput[j]=delayInput[j-1];
				  } // shift loop

			    // Clipping
			    if(sum > FRACT_MAX){
			        sum = FRACT_MAX;
			    } else if (sum < FRACT_MIN){
			        sum = FRACT_MIN;
			    } // Clipping if statements

				for(i=0;i<NUM_COEFF;i++) {
					control_coeffs[i] = control_coeffs[i] + Delta * (delayError[i]) * (fract)sum; //or long fract if doesnt work
				} // LMS

			    // Convolution
			    sum = 0.0k;
			    for(i=0; i<NUM_COEFF; i++){
			        sum += delayError[i]*control_coeffs[i];
			    } // Convolution loop

			    // shift delay line
			    for(j=NUM_COEFF-1;j>0;j--){
					    delayError[j]=delayError[j-1];
				  } // shift loop

			    // Clipping
			    if(sum > FRACT_MAX){
			        sum = FRACT_MAX;
			    } else if (sum < FRACT_MIN){
			        sum = FRACT_MIN;
			    } // Clipping if statements

filterOutput[0] = bitslr((long fract)(sum)>>8);
filterOutput[1] = bitslr((long fract)(sum)>>8);
