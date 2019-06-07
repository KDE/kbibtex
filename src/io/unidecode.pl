#!/usr/bin/env perl

# The Perl module 'Text::Unidecode' allows to transliterate Unicode text into
# plain ASCII, i.e. sequences of single-byte symbols in the range 0x00..0x7f.
#
#   https://metacpan.org/pod/Text::Unidecode
#
# This Perl module was also the base for a number of similar modules/packages
# in other programming/scripting languages. For example, several re-
# implementations exist for Python.
#
#   https://pypi.org/project/Unidecode/
#   https://pypi.org/project/text-unidecode/
#   https://pypi.org/project/isounidecode/
#
# This Perl script here makes use of the 'Text::Unidecode' module's sources
# without actually 'use'ing it. To run this script, you will need the sources
# for 'Text::Unidecode' from CPAN, extracted in a temporary directory.
# Adjust variable $basedir to point to the right location.
#
# This Perl script here will read the translation tables from the Perl module
# and generate a C code fragment with two variables: unidecode_pos and
# unidecode_text.
# unidecode_pos is an array of 65536 unsigned integers, one for each of the
# 65536 first Unicode symbols. Each value in this array encodes a position
# and a length of a substring in unidecode_text to extract an ASCII
# representation. The position and the length get computed as shown in the
# following pseudo-code:
#
#  unsigned int unicode = 0x00e4; ///< Latin small letter A with diaeresis
#  const unsigned int encodedposlen = unidecode_pos[unicode];
#  /// In this particular example: encodedposlen == 2081
#  const unsigned int pos = encodedposlen >> 5;
#  /// In this particular example: pos == 65
#  const unsigned int len = encodedposlen & 31;
#  /// In this particular example: pos == 1
#  /// Choose one of the following two alternatives, both will be "a"
#  const char *cascii = strndup(unidecode_text + pos, len);
#  const QString qascii = QString::fromLatin1(unidecode_text + pos, len);

use strict;
use warnings;

my $basedir = "/tmp/Text-Unidecode-1.30/";
my $xpmdir  = "lib/Text/Unidecode/";

my @list   = ();
my @pos    = ();
my $text   = "";
my $maxlen = 0;

for ( my $high = 0 ; $high < 256 ; ++$high ) {
    open( my $fh, "<", $basedir . $xpmdir . sprintf( "x%02x.pm", $high ) )
      || die "Could not open file: $!";
    my $file_content = do { local $/; <$fh> };
    close($fh);

    $file_content =~ s/\$Text::Unidecode::/\@/g;
    $file_content =~ s/Char\[0x[0-9A-Fa-f]+\]/list/g;
    eval $file_content;

    for ( my $low = 0 ; $low < 256 ; ++$low ) {
        my $curtext = $list[0][$low];
        chomp($curtext);
        $curtext = ""
          if (
            length($curtext) == 1
            && (   ord( substr( $curtext, 0, 1 ) ) < 32
                || ord( substr( $curtext, 0, 1 ) ) > 127 )
          );
        my $curtextlen = length($curtext);
        $maxlen = $curtextlen if ( $maxlen < $curtextlen );
        my $curtextpos = $curtextlen > 0 ? index( $text, $curtext ) : 0;
        if ( $curtextpos < 0 ) {
            $curtextpos = length($text);
            $text .= $curtext;
        }
        my $value = ( $curtextlen & 31 ) | ( $curtextpos << 5 );

        die "Text is too long, adjust script!" if ( $curtextlen > 31 );
        push( @pos, $value );
    }
}

$text =~ s/\\/\\\\/g;
$text =~ s/"/\\"/g;

print("static const unsigned int unidecode_pos[] = {");
for ( my $i = 0 ; $i <= $#pos ; ++$i ) {
    print "," if ( $i > 0 );
    if ( $i % 16 == 0 ) {
        print("\n    ");
    }
    else {
        print(" ");
    }
    print $pos[$i];
}
print("\n};\n");
print("static const char *unidecode_text = ");
while ( length($text) > 80 ) {
    print( "\n        \"" . substr( $text, 0, 64 ) . "\"" );
    $text = substr( $text, 64 );
}
print(";\n");
