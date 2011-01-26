/*
  ==============================================================================

   This file is part of the JUCE library - "Jules' Utility Class Extensions"
   Copyright 2004-10 by Raw Material Software Ltd.

  ------------------------------------------------------------------------------

   JUCE can be redistributed and/or modified under the terms of the GNU General
   Public License (Version 2), as published by the Free Software Foundation.
   A copy of the license is included in the JUCE distribution, or can be found
   online at www.gnu.org/licenses.

   JUCE is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
   A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

  ------------------------------------------------------------------------------

   To release a closed-source product which uses JUCE, commercial licenses are
   available: visit www.rawmaterialsoftware.com/juce for more information.

  ==============================================================================
*/

#ifndef __JUCE_CHARACTERFUNCTIONS_JUCEHEADER__
#define __JUCE_CHARACTERFUNCTIONS_JUCEHEADER__


//==============================================================================
#if JUCE_ANDROID && ! DOXYGEN
 typedef uint32                     juce_wchar;
 #define JUCE_T(stringLiteral)      CharPointer_UTF8 (stringLiteral)
#else
 /** A platform-independent unicode character type. */
 typedef wchar_t                    juce_wchar;
 #define JUCE_T(stringLiteral)      (L##stringLiteral)
#endif

#if ! JUCE_DONT_DEFINE_MACROS
 /** The 'T' macro allows a literal string to be compiled as unicode.

     If you write your string literals in the form T("xyz"), it will be compiled as L"xyz"
     or "xyz", depending on which representation is best for the String class to work with.

     Because the 'T' symbol is occasionally used inside 3rd-party library headers which you
     may need to include after juce.h, you can use the juce_withoutMacros.h file (in
     the juce/src directory) to avoid defining this macro. See the comments in
     juce_withoutMacros.h for more info.
 */
 #define T(stringLiteral)            JUCE_T(stringLiteral)
#endif

#undef max
#undef min

//==============================================================================
/**
    A set of methods for manipulating characters and character strings.

    These are defined as wrappers around the basic C string handlers, to provide
    a clean, cross-platform layer, (because various platforms differ in the
    range of C library calls that they provide).

    @see String
*/
class JUCE_API  CharacterFunctions
{
public:
    //==============================================================================
    static juce_wchar toUpperCase (juce_wchar character) throw();
    static juce_wchar toLowerCase (juce_wchar character) throw();

    static bool isUpperCase (juce_wchar character) throw();
    static bool isLowerCase (juce_wchar character) throw();

    static bool isWhitespace (char character) throw();
    static bool isWhitespace (juce_wchar character) throw();

    static bool isDigit (char character) throw();
    static bool isDigit (juce_wchar character) throw();

    static bool isLetter (char character) throw();
    static bool isLetter (juce_wchar character) throw();

    static bool isLetterOrDigit (char character) throw();
    static bool isLetterOrDigit (juce_wchar character) throw();

    /** Returns 0 to 16 for '0' to 'F", or -1 for characters that aren't a legal hex digit. */
    static int getHexDigitValue (juce_wchar digit) throw();

    //==============================================================================
    template <typename CharPointerType>
    static double getDoubleValue (const CharPointerType& text) throw()
    {
        double result[3] = { 0, 0, 0 }, accumulator[2] = { 0, 0 };
        int exponentAdjustment[2] = { 0, 0 }, exponentAccumulator[2] = { -1, -1 };
        int exponent = 0, decPointIndex = 0, digit = 0;
        int lastDigit = 0, numSignificantDigits = 0;
        bool isNegative = false, digitsFound = false;
        const int maxSignificantDigits = 15 + 2;

        CharPointerType s (text.findEndOfWhitespace());
        juce_wchar c = *s;

        switch (c)
        {
            case '-':   isNegative = true; // fall-through..
            case '+':   c = *++s;
        }

        switch (c)
        {
            case 'n':
            case 'N':
                if ((s[1] == 'a' || s[1] == 'A') && (s[2] == 'n' || s[2] == 'N'))
                    return std::numeric_limits<double>::quiet_NaN();
                break;

            case 'i':
            case 'I':
                if ((s[1] == 'n' || s[1] == 'N') && (s[2] == 'f' || s[2] == 'F'))
                    return std::numeric_limits<double>::infinity();
                break;
        }

        for (;;)
        {
            if (s.isDigit())
            {
                lastDigit = digit;
                digit = s.getAndAdvance() - '0';
                digitsFound = true;

                if (decPointIndex != 0)
                    exponentAdjustment[1]++;

                if (numSignificantDigits == 0 && digit == 0)
                    continue;

                if (++numSignificantDigits > maxSignificantDigits)
                {
                    if (digit > 5)
                        ++accumulator [decPointIndex];
                    else if (digit == 5 && (lastDigit & 1) != 0)
                        ++accumulator [decPointIndex];

                    if (decPointIndex > 0)
                        exponentAdjustment[1]--;
                    else
                        exponentAdjustment[0]++;

                    while (s.isDigit())
                    {
                        ++s;
                        if (decPointIndex == 0)
                            exponentAdjustment[0]++;
                    }
                }
                else
                {
                    const double maxAccumulatorValue = (double) ((std::numeric_limits<unsigned int>::max() - 9) / 10);
                    if (accumulator [decPointIndex] > maxAccumulatorValue)
                    {
                        result [decPointIndex] = mulexp10 (result [decPointIndex], exponentAccumulator [decPointIndex])
                                                    + accumulator [decPointIndex];
                        accumulator [decPointIndex] = 0;
                        exponentAccumulator [decPointIndex] = 0;
                    }

                    accumulator [decPointIndex] = accumulator[decPointIndex] * 10 + digit;
                    exponentAccumulator [decPointIndex]++;
                }
            }
            else if (decPointIndex == 0 && *s == '.')
            {
                ++s;
                decPointIndex = 1;

                if (numSignificantDigits > maxSignificantDigits)
                {
                    while (s.isDigit())
                        ++s;
                    break;
                }
            }
            else
            {
                break;
            }
        }

        result[0] = mulexp10 (result[0], exponentAccumulator[0]) + accumulator[0];

        if (decPointIndex != 0)
            result[1] = mulexp10 (result[1], exponentAccumulator[1]) + accumulator[1];

        c = *s;
        if ((c == 'e' || c == 'E') && digitsFound)
        {
            bool negativeExponent = false;

            switch (*++s)
            {
                case '-':   negativeExponent = true; // fall-through..
                case '+':   ++s;
            }

            while (s.isDigit())
                exponent = (exponent * 10) + (s.getAndAdvance() - '0');

            if (negativeExponent)
                exponent = -exponent;
        }

        double r = mulexp10 (result[0], exponent + exponentAdjustment[0]);
        if (decPointIndex != 0)
            r += mulexp10 (result[1], exponent - exponentAdjustment[1]);

        return isNegative ? -r : r;
    }

    //==============================================================================
    template <typename IntType, typename CharPointerType>
    static IntType getIntValue (const CharPointerType& text) throw()
    {
        IntType v = 0;
        CharPointerType s (text.findEndOfWhitespace());

        const bool isNeg = *s == '-';
        if (isNeg)
            ++s;

        for (;;)
        {
            const juce_wchar c = s.getAndAdvance();

            if (c >= '0' && c <= '9')
                v = v * 10 + (IntType) (c - '0');
            else
                break;
        }

        return isNeg ? -v : v;
    }

    //==============================================================================
    static int ftime (char* dest, int maxChars, const char* format, const struct tm* tm) throw();
    static int ftime (juce_wchar* dest, int maxChars, const juce_wchar* format, const struct tm* tm) throw();

    //==============================================================================
    template <typename DestCharPointerType, typename SrcCharPointerType>
    static void copyAndAdvance (DestCharPointerType& dest, SrcCharPointerType src) throw()
    {
        juce_wchar c;

        do
        {
            c = src.getAndAdvance();
            dest.write (c);
        }
        while (c != 0);
    }

    template <typename DestCharPointerType, typename SrcCharPointerType>
    static int copyAndAdvanceUpToBytes (DestCharPointerType& dest, SrcCharPointerType src, int maxBytes) throw()
    {
        int numBytesDone = 0;

        for (;;)
        {
            const juce_wchar c = src.getAndAdvance();
            const size_t bytesNeeded = DestCharPointerType::getBytesRequiredFor (c);

            maxBytes -= bytesNeeded;
            if (maxBytes < 0)
                break;

            numBytesDone += bytesNeeded;
            dest.write (c);
            if (c == 0)
                break;
        }

        return numBytesDone;
    }

    template <typename DestCharPointerType, typename SrcCharPointerType>
    static void copyAndAdvanceUpToNumChars (DestCharPointerType& dest, SrcCharPointerType src, int maxChars) throw()
    {
        while (--maxChars >= 0)
        {
            const juce_wchar c = src.getAndAdvance();
            dest.write (c);
            if (c == 0)
                break;
        }
    }

    template <typename CharPointerType1, typename CharPointerType2>
    static int compare (CharPointerType1 s1, CharPointerType2 s2) throw()
    {
        for (;;)
        {
            const int c1 = (int) s1.getAndAdvance();
            const int c2 = (int) s2.getAndAdvance();

            const int diff = c1 - c2;
            if (diff != 0)
                return diff < 0 ? -1 : 1;
            else if (c1 == 0)
                break;
        }

        return 0;
    }

    template <typename CharPointerType1, typename CharPointerType2>
    static int compareUpTo (CharPointerType1 s1, CharPointerType2 s2, int maxChars) throw()
    {
        while (--maxChars >= 0)
        {
            const int c1 = (int) s1.getAndAdvance();
            const int c2 = (int) s2.getAndAdvance();

            const int diff = c1 - c2;
            if (diff != 0)
                return diff < 0 ? -1 : 1;
            else if (c1 == 0)
                break;
        }

        return 0;
    }

    template <typename CharPointerType1, typename CharPointerType2>
    static int compareIgnoreCase (CharPointerType1 s1, CharPointerType2 s2) throw()
    {
        for (;;)
        {
            int c1 = s1.toUpperCase();
            int c2 = s2.toUpperCase();
            ++s1;
            ++s2;

            const int diff = c1 - c2;
            if (diff != 0)
                return diff < 0 ? -1 : 1;
            else if (c1 == 0)
                break;
        }

        return 0;
    }

    template <typename CharPointerType1, typename CharPointerType2>
    static int compareIgnoreCaseUpTo (CharPointerType1 s1, CharPointerType2 s2, int maxChars) throw()
    {
        while (--maxChars >= 0)
        {
            int c1 = s1.toUpperCase();
            int c2 = s2.toUpperCase();
            ++s1;
            ++s2;

            const int diff = c1 - c2;
            if (diff != 0)
                return diff < 0 ? -1 : 1;
            else if (c1 == 0)
                break;
        }

        return 0;
    }

    template <typename CharPointerType1, typename CharPointerType2>
    static int indexOf (CharPointerType1 haystack, const CharPointerType2& needle) throw()
    {
        int index = 0;
        const int needleLength = needle.length();

        for (;;)
        {
            if (haystack.compareUpTo (needle, needleLength) == 0)
                return index;

            if (haystack.getAndAdvance() == 0)
                return -1;

            ++index;
        }
    }

    template <typename Type>
    static int indexOfChar (Type text, const juce_wchar charToFind) throw()
    {
        int i = 0;

        while (! text.isEmpty())
        {
            if (text.getAndAdvance() == charToFind)
                return i;

            ++i;
        }

        return -1;
    }

    template <typename Type>
    static int indexOfCharIgnoreCase (Type text, juce_wchar charToFind) throw()
    {
        charToFind = CharacterFunctions::toLowerCase (charToFind);
        int i = 0;

        while (! text.isEmpty())
        {
            if (text.toLowerCase() == charToFind)
                return i;

            ++text;
            ++i;
        }

        return -1;
    }

    template <typename Type>
    static Type findEndOfWhitespace (const Type& text) throw()
    {
        Type p (text);

        while (p.isWhitespace())
            ++p;

        return p;
    }

private:
    static double mulexp10 (const double value, int exponent) throw();
};


#endif   // __JUCE_CHARACTERFUNCTIONS_JUCEHEADER__
