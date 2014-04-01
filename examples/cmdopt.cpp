#include <string>
#include <vector>
#include <iostream>

#include <opts/opts.h>

int main(int argc, char** argv)
{
    using opts::Option;
    using opts::Present;
    using opts::PosOption;

    opts::Options     ops(argc, argv);

    std::string                     name    = "Pepper";
    unsigned int                    age     = 7;
    double                          area    = 1.0;
    std::vector<std::string>        coordinates;
    ops
        >> Option(      "name",     name,           "name of the person")
        >> Option('a',  "age",      age,            "age of the person")
        >> Option(      "area",     area,           "some area")
        >> Option('c',  "coord",    coordinates,    "coordinates")
    ;
    bool negate = ops >> Present('n', "negate", "negate the function");

    // NB: PosOptions must be read last
    std::string infilename, outfilename;
    if ( (ops >> Present('h', "help", "show help message")) ||
        !(ops >> PosOption(infilename) >> PosOption(outfilename)))
    {
        std::cout << "Usage: " << argv[0] << " [options] INFILE OUTFILE\n\n";
        std::cout << "Sample options program\n\n";
        std::cout << ops << std::endl;
        return 1;
    }

    std::cout << "Infilename:  " << infilename  << std::endl;
    std::cout << "Outfilename: " << outfilename << std::endl;
    std::cout << "Name:        " << name        << std::endl;
    std::cout << "Area:        " << area        << std::endl;
    std::cout << "Age:         " << age         << std::endl;
    std::cout << "Negate:      " << negate      << std::endl;
    std::cout << "Coorindates: " << std::endl;
    for (unsigned i = 0; i < coordinates.size(); ++i)
        std::cout << "  " << coordinates[i] << std::endl;
}
