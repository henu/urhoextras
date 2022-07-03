#ifndef URHOEXTRAS_DEBUG_ASCIITABLE_HPP
#define URHOEXTRAS_DEBUG_ASCIITABLE_HPP

#include <Urho3D/Container/Str.h>
#include <Urho3D/Container/Vector.h>

namespace UrhoExtras
{

namespace Debug
{

class AsciiTable
{

public:

    enum Alignment {
        A_LEFT,
        A_CENTER,
        A_RIGHT
    };

    void addCell(Urho3D::String const& str, Alignment align = A_LEFT);
    void endRow();

    Urho3D::String toString() const;

private:

    struct Cell
    {
        Urho3D::String str;
        Alignment align;

        inline Cell(Urho3D::String const& str, Alignment align) :
            str(str),
            align(align)
        {
        }

        inline Cell() :
            align(A_LEFT)
        {
        }
    };
    typedef Urho3D::Vector<Cell> Row;
    typedef Urho3D::Vector<Row> Table;
    typedef Urho3D::Vector<unsigned> Widths;

    Table table;

};

}

}

#endif
