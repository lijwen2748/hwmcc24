#ifndef DRAWING_H
#define DRAWING_H

#include "data_structure.h"


void print_U_sequnece(const Usequence & U, ostream & out_stream)
{
    int B_level = 0;
    out_stream << "------ Start Printing U sequence ------" << endl;
    for (auto frame : U)
    {
        out_stream << "U level : " << B_level++ << endl;
        for (auto state : frame)
        {
            out_stream << "(";
            for (auto lit : state->s())
            {
                out_stream << (lit > 0 ? '+' : '-');
            }
            out_stream << "), id = "<<state->id << endl;
        }
        out_stream << endl;
    }
    out_stream << "------ End Printing U sequence ------" << endl << endl;
}


void print_O_sequence(const Osequence &O, const Frame& Otmp, ostream & out_stream)
{
    int O_level = 0;
    out_stream << "------ Start Printing O sequence ------" << endl;
    for (auto frame : O)
        {
            out_stream << "O level : " << O_level++ << endl;
            for (auto cube : frame)
            {
                out_stream << "(";
                for (auto lit : cube)
                {
                    out_stream << lit << ", ";
                }
                out_stream << ")," << endl;
            }
            out_stream << endl;
        }
    {
        out_stream << "O level : " << "Otmp" << endl;
        for (auto cube : Otmp)
        {
            out_stream << "(";
            for (auto lit : cube)
            {
                out_stream << lit << ", ";
            }
            out_stream << ")," << endl;
        }
        out_stream << endl;
    }

    out_stream << "------ End Printing O sequence ------" << endl;
}

#endif // !DRAWING_H
