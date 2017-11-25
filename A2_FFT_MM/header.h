//
//  header.h
//  A2_FFT_MM
//
//  Created by Matthew Mansell on 24/11/2017.
//  Copyright © 2017 Matthew Mansell. All rights reserved.
//

#ifndef header_h
#define header_h

/* Utility function to copy an arrays content.
 */
int copyArray(int srcArray[], int destArray[]) {
    if(sizeof(*srcArray) != sizeof(*destArray)) {
        //Array sizes dont match
        return 0;
    } else {
        for(int indx = 0; indx < sizeof(*srcArray); indx++) {
            destArray[indx] = srcArray[indx];
        }
        return 1;
    }
}


#endif /* header_h */
