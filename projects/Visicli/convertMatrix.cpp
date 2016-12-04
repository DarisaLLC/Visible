/*
 *
 *$Id $
 *$Log: $
 *
 * Copyright (c) 2007 Reify Corp. All rights reserved.
 */

#include <stlplus.hpp>
#include <textio.hpp>
#include <fstream>
#include <iostreamio.hpp>
#include <rc_types.h>
#include <rc_fileutils.h>
#include <rc_vector2d.h>
#include <rc_macro.h>
#include <matrix.hpp>

using namespace std;

int main(int argc, char** argv)
{
  if (argc < 3) exit (0);
	std::string outf = string (argv[1]);
	std::string filef = string (argv[2]);

  try
    {
      iftext input (outf);
      matrix<double> cm;
      restore_context restorer (input);
      restore_matrix (restorer, cm);

			ofstream output_stream(filef.c_str(), ios::trunc);
      // create and initialise the TextIO wrapper device
      oiotext output(output_stream);
      // now use the device
      uint32 i, j;
      for (i = 0; i < cm.size(); i++) {
	deque<double>::iterator ds = cm[i].begin();
	for (j = 0; j < cm.size() - 1; j++)
	  output << *ds++ << ",";
	output << *ds << endl;
      }
      output_stream.flush();  
    }
}
