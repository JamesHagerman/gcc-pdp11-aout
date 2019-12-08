int start() {
  int *con = (int *)0177570; // Display register
  unsigned int count = 0;
  *con = 0125252; // Value to put in that register
  while (1) {
    count++;
    *con = count;
  }
  return 0;
}
