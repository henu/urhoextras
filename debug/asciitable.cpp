#include "asciitable.hpp"

#include <Urho3D/Math/MathDefs.h>

namespace UrhoExtras
{

namespace Debug
{

void AsciiTable::addCell(Urho3D::String const& str, Alignment align)
{
    if (table.Empty()) {
        table.Push(Row());
    }

    table.Back().Push(Cell(str, align));
}

void AsciiTable::endRow()
{
    table.Push(Row());
}

Urho3D::String AsciiTable::toString() const
{
    // First calculate column widths
    Widths widths;
    for (Row const& row : table) {
        for (unsigned col = 0; col < row.Size(); ++ col) {
            if (widths.Size() <= col) {
                widths.Push(0);
            }
            widths[col] = Urho3D::Max(widths[col], row[col].str.Length());
        }
    }

    // Then create the table
    Urho3D::String result;

    for (Row const& row : table) {
        for (unsigned col = 0; col < widths.Size(); ++ col) {
            // Get cell content and calculate paddings
            Urho3D::String cell;
            unsigned left_pad, right_pad;
            if (col < row.Size()) {
                cell = row[col].str;
                if (row[col].align == A_LEFT) {
                    left_pad = 0;
                    right_pad = widths[col] - cell.Length();
                } else if (row[col].align == A_RIGHT) {
                    left_pad = widths[col] - cell.Length();
                    right_pad = 0;
                } else {
                    left_pad = (widths[col] - cell.Length()) / 2;
                    right_pad = widths[col] - cell.Length() - left_pad;
                }
            } else {
                left_pad = 0;
                right_pad = widths[col];
            }
            // Left padding
            for (unsigned i = 0; i < left_pad; ++ i) result += ' ';
            // Left padding
            result += cell;
            // Right padding
            for (unsigned i = 0; i < right_pad; ++ i) result += ' ';
            // Column separator
            if (col < widths.Size() - 1) {
                result += ' ';
            }
        }
        result += '\n';
    }

    return result;
}

}

}
