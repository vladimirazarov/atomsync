#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/* Macros */
#define STATS_STRING "--stats" // string for --stats parameter
#define PASS_LENGTH 100        // maximum length of the one password

/* Get length of an array of characters not including new line character */
int getStrLen(char string[]) {
  int i;
  for (i = 0; string[i] != '\0' && string[i] != '\n'; i++) {
    ;
  }
  return i;
}

/* Check if a character is already inside of an array of characters */
int isInSet(char set[], char value) {
  for (int i = 0; set[i] != '\0'; i++) {
    if (set[i] == value)
      return 1;
  }
  return 0;
}

/* Add character to an array of characters if it is not already inside of this
 * array */
void addChar(char *arr, char *str) {

  if (arr[0] == '\0') {
    arr[0] = str[0];
    arr[1] = '\0';
  }

  int i, n, k;

  k = getStrLen(arr);

  for (i = 0; arr[i] != '\0'; i++) {
    for (n = 0; str[n] != '\0'; n++) {
      if ((isInSet(arr, str[n]) == 0) && (str[n] >= 32 && str[n] <= 126)) {
        arr[k] = str[n];
        arr[k + 1] = '\0';
        k++;
      }
    }
  }
}

/* Return 1 if string is full of digits.
 * Otherwise return 0 if at least one letter or other non-digit character is met
 */
int isDigit(char *str) {
  int i;
  for (i = 0; str[i] != '\0'; i++) {
    if (str[i] < '0' || str[i] > '9') {
      return 0;
    }
  }
  return 1;
}

/* Strings are the same - return 0, strings are different return 1 */
int compareStrings(char str1[], char str2[]) {
  int i;
  for (i = 0; str1[i] != '\0' || str2[i] != '\0'; i++) {
    if (str1[i] != str2[i]) {
      return 1;
    }
  }
  return 0;
}

/* Minimum password length update function */
void updateMin(char buffer[], unsigned short int *min) {

  int len = getStrLen(buffer);

  if (*min == 0 && len != 0) {
    *min = len;
  } else if (len < *min) {
    *min = len;
  } else
    return;
}

/* Input data control */
int inputParamsControl(int level, long long int param, int argc, char *opt) {
  if (level > 4 || level < 1) {
    fprintf(stderr, "Invalid argument \"level\"\n");
    return 1;
  } else if (param < 1 && param >= 0) {
    fprintf(stderr, "Invalid argument \"param\"\n");
    return 1;
  }

  else if (opt != NULL && compareStrings(opt, STATS_STRING) == 1) {
    fprintf(stderr, "Invalid string argument\n");
    return 1;
  } else if (argc < 3 || argc > 4) {
    fprintf(stderr, "The wrong number of arguments\n");
    return 1;
  } else
    return 0;
}

int lvl1Check(char str[])
{
  int i;
  unsigned short int upperCase, lowerCase; 
  lowerCase = upperCase = 0;

  for (i = 0; str[i] != '\0'; i++) {
    if (str[i] >= 'A' && str[i] <= 'Z') {
      upperCase = 1;
    } else if (str[i] >= 'a' && str[i] <= 'z') {
      lowerCase = 1;
    }
  }
  if (lowerCase == 1 && upperCase == 1) {
    return 0;
  } else {
    return 1;
  }
}

int lvl2Check(char str[], int param) {
  int i;
  int upperCase, lowerCase, number, specialSymbol; 
  upperCase = lowerCase = number = specialSymbol = 0;

  for (i = 0; str[i] != '\0'; i++) {
    if (str[i] >= 'A' && str[i] <= 'Z') {
      upperCase = 1;
    } else if (str[i] >= 'a' && str[i] <= 'z') {
      lowerCase = 1;
    } else if (str[i] >= '0' && str[i] <= '9') {
      number = 1;
    } else if (str[i] >= 32 && str[i] <= 126) {
      specialSymbol = 1;
    }
  }
  if ((upperCase + lowerCase + number + specialSymbol) >= param) {
    return 0;
  } else {
    return 1;
  }
}

int lvl3Check(char str[], int param) {
  int i;
  int sameLetterCounter, maxLetterSequence;
  sameLetterCounter = 0;
  maxLetterSequence = 0;

  for (i = 0;; i++) {
    if (str[i] == '\0') {
      maxLetterSequence++; // take care of the last letter
      break;
    } else if (str[i] == str[i + 1]) {
      sameLetterCounter++;
    } else {
      maxLetterSequence = sameLetterCounter;
    }
  }

  if (maxLetterSequence < param) {
    return 0;
  } else {
    return 1;
  }
}

int lvl4Check(char str[], int param) {

  int subStrCounter; // returned variable
  int startingPoint; // where we start to fill Substring array
  char subStr[param + 1];
  subStrCounter = startingPoint = 0;

  while (str[startingPoint] != '\0') {
    int i, k, n;
    int tempCounter = 0; //

    /* fill up Substring array */
    for (i = 0; i < param; i++) {
      subStr[i] = str[startingPoint + i];
    }

    subStr[i] = '\0';
    
    for (i = 0; str[i] != '\0'; i++) 
    {
      for (k = 0, n = i;
           (subStr[k] == str[n]) && (subStr[k] != '\0' && str[n] != '\0');
           k++, n++) {
        ;
      }

      if (k == param) {
        tempCounter++;
      }
      if (tempCounter > subStrCounter) {
        subStrCounter = tempCounter;
      }
    }

    startingPoint++;
  }

  return subStrCounter;
}

int main(int argc, char *argv[]) {
  long long int param;
  int level;
  char *opt;
  char buffer[PASS_LENGTH + 2];
  int i, c;
  unsigned short int min;
  unsigned short int *pmin;
  unsigned int passCount;
  passCount = 0;
  pmin = &min;
  double avg;
  avg = min = 0;

  opt = argv[3];
  char diffChar[100];
  diffChar[0] = '\0';

  if (argv[1] == NULL || argv[2] == NULL) {
    fprintf(stderr, "Not enough arguments\n");
    return 1;
  } else if (isDigit(argv[1]) == 0 || isDigit(argv[2]) == 0) {
    fprintf(stderr, "Arguments can be only digits\n");
    return 1;
  } else {
    level = atoi(argv[1]);
    param = atoi(argv[2]);
  }

  if (inputParamsControl(level, param, argc, opt) == 1)
    return 1;

  else if (param > LONG_MAX || param < 0) {
    return 0;
  }

  while (!feof(stdin)) {
    for (i = 0; (c = getchar()) != EOF && c != '\n'; i++) {
      if (i + 1 > PASS_LENGTH) {
        fprintf(stderr, "Password is too long\n");
        return 1;
      }
      buffer[i] = c; // fill the buffer
    }
    buffer[i] = c;
    buffer[i + 1] = '\0';
    if (c != EOF) {
      addChar(diffChar, buffer);
      updateMin(buffer, pmin);
      avg += getStrLen(buffer);
      passCount++;
    }

    switch (level) {
    case 1:
      if (lvl1Check(buffer) == 0) {
        printf("%s", buffer);
      }
      break;
    case 2:
      if (lvl1Check(buffer) == 0 && lvl2Check(buffer, param) == 0) {
        printf("%s", buffer);
      }
      break;
    case 3:
      if (lvl1Check(buffer) == 0 && lvl2Check(buffer, param) == 0 &&
          lvl3Check(buffer, param) == 0) {
        printf("%s", buffer);
      }
      break;
    case 4:
      if (lvl1Check(buffer) == 0 && lvl2Check(buffer, param) == 0 &&
          lvl3Check(buffer, param) == 0 && lvl4Check(buffer, param) < 2) {
        printf("%s", buffer);
      }
      break;
    }
  }

  if (opt != NULL) {
    printf("Statistika:\n");
    printf("Ruznych znaku: %d\n", getStrLen(diffChar));
    printf("Minimalni delka: %d\n", min);
    printf("Prumerna delka: %.1f\n", (avg / passCount));
  }

  return 0;
}
