#!/usr/bin/perl -w

#
# Author: Alexander
# Email: solar@openwall.com
# Date: 2015-06-05
#

#
# How to run
#
# ulimit -v 2097152; time ./fuzz.pl /path/to/john format-name &> fuzz.log
#

use Errno;

# Processes per logical CPU
$factor = 4;

$workdir = '/dev/shm/fuzz';
$pwfile = $workdir . '/pw';
$session = $workdir . '/s';
$pot = $workdir . '/pot';
$fuzz_method = "";

die unless (mkdir($workdir, 0700) || $!{EEXIST});

$ENV{'OMP_NUM_THREADS'} = '1';
setpriority(PRIO_PROCESS, 0, 19);

if (1 > $#ARGV || 2 < $#ARGV) {
    print "usage: ./fuzz.pl /path/to/john format-name [dictionary]\n";
    die;
}

$john_path = $ARGV[0];
$format_name = $ARGV[1];
$dictionary_file = "";

if (2 == $#ARGV) {
	$dictionary_file = $ARGV[2];

	#
	# Init dictionary file
	#
	open($dic_handler, "<", $dictionary_file)
		|| die "dictionary_file=$dictionary_file does not exist\n";
	while ($line = <$dic_handler>) {
		chomp $line;
		$dictionary[$#dictionary + 1] = $line;
	}
	close($dic_handler);
}

#
# Get all test vector
#
open(TESTS, "$john_path --list=format-tests --format=$format_name | shuf |") || die;
while (<TESTS>) {
	($f, $c) = /^([\w\d-]+)\t[^\t]+\t([^\t]+)\t/;
	if ($f && $c) {
		$fs[$#fs + 1] = $f;
		$cs[$#cs + 1] = $c;
	}
}
close(TESTS);

die unless ($#fs >= 0 && $#fs == $#cs);

printf "%u test vectors\n", $#fs + 1;

$cpus = `grep -c ^processor /proc/cpuinfo`;
chomp $cpus;
$cpus = 1 if (!$cpus);

print "$cpus CPUs\n";

$cpus *= $factor;

for ($cpu = 0; $cpu < $cpus - 1; $cpu++) {
	last if (fork() == 0);
}

$from = int(($#fs + 1) * $cpu / $cpus);
$to = int(($#fs + 1) * ($cpu + 1) / $cpus) - 1;

$session .= "-$cpu";
$pot .= "-$cpu";
$pwfile .= "-$cpu";

$seq = 0;

for ($t = $from; $t <= $to; $t++) {
	$f = $fs[$t];
	$c = $cs[$t];
	$o = $c;

	print "\n";
	print "format        = $f\n";
	print "original hash = $o\n";

	$fuzz_method = "raw";
	Run();

        print "\n";
        print "->9\n\n";
	# Replace chars with '9'
	for ($i = 0; $i < length($c); $i++) {
		vec($c, $i, 8) = ord('9');
		next if ($c eq $o);
		#print "new=$c\n";
		$fuzz_method = "Replace $i with 9";
		Run();
		$c = $o;
	}

	print "\n";
	print "->\$\n\n";
	# Replace chars with '$'
	for ($i = 0; $i < length($c); $i++) {
		vec($c, $i, 8) = ord('$');
		next if ($c eq $o);
		#print "new=$c\n";
		$fuzz_method = "Replace $i with \$";
		Run();
		$c = $o;
	}

	print "\n";
	print "->*\n\n";
	# Replace chars with '*'
	for ($i = 0; $i < length($c); $i++) {
		vec($c, $i, 8) = ord('*');
		next if ($c eq $o);
		#print "new=$c\n";
		$fuzz_method = "Replace $i with \*";
		Run();
		$c = $o;
	}

	print "\n";
	print "->#\n\n";
	# Replace chars with '#'
	for ($i = 0; $i < length($c); $i++) {
		vec($c, $i, 8) = ord('#');
		next if ($c eq $o);
		#print "new=$c\n";
		$fuzz_method = "Replace $i with \#";
		Run();
		$c = $o;
	}

	print "\n";
	print "swap two adjacent chars\n\n";
	# Swap two adjacent chars
	for ($i = 0; $i < length($c) - 1; $i++) {
		my $x = vec($c, $i, 8);
		vec($c, $i, 8) = vec($c, $i + 1, 8);
		vec($c, $i + 1, 8) = $x;
		next if ($c eq $o);
		#print "new=$c\n";
		$fuzz_method = "Swap $i";
		Run();
		$c = $o;
	}

	print "\n";
	print "Append many copies of the last char\n\n";
	# Append many copies of the last char
	$j = 1;
	for ($i = 0; $i < 5; $i++) {
		print "j=$j\n";
		$c = $o . chr(vec($o, length($o) - 1, 8)) x $j;
		#print "new=$c\n";
		$fuzz_method = "Append $j last char";
		Run();
		$j *= ($j + 1);
	}

	ChangeCase();

	if ($dictionary_file eq "") {
	} else {
		InsertDictionary();
        }
}

#
# 1. Change each char to upper case and lower case
# 2. Change all chars to upper case and lower case
#
sub ChangeCase
{
	print "\nChangecase\n\n";

	$c =$o;

	# Change chars to Upper Case
	for ($i = 0; $i < length($c); $i++) {
		$char = chr(vec($c, $i, 8));
		if ($char =~ /[a-z]/ || $char =~ /[A-Z]/) {
			$char = uc $char;
			vec($c, $i, 8) = vec($char, 0, 8);
			next if ($c eq $o);
			#print "new=$c\n";
			$fuzz_method = "Change $i to uppper case";
			Run();
		}
		$c = $o;
	}

	print "\n";
	# Change chars to lower Case
	for ($i = 0; $i < length($c); $i++) {
		$char = chr(vec($c, $i, 8));
		if ($char =~ /[a-z]/ || $char =~ /[A-Z]/) {
			$char = lc $char;
			vec($c, $i, 8) = vec($char, 0, 8);
			next if ($c eq $o);
			#print "new=$c\n";
			$fuzz_method = "Change $i to lower case";
			Run();
		}
		$c = $o;
	}

	print "\n";
	# Change all chars to upper Case
	for ($i = 0; $i < length($c); $i++) {
		$char = chr(vec($c, $i, 8));
		if ($char =~ /[a-z]/ || $char =~ /[A-Z]/) {
			$char = uc $char;
			vec($c, $i, 8) = vec($char, 0, 8);
		}
	}
	#print "new=$c\n";
	$fuzz_method = "Change all to upper case";
	Run();

	print "\n";
	# Change all chars to lower Case
	for ($i = 0; $i < length($c); $i++) {
		$char = chr(vec($c, $i, 8));
		if ($char =~ /[a-z]/ || $char =~ /[A-Z]/) {
			$char = lc $char;
			vec($c, $i, 8) = vec($char, 0, 8);
		}
	}
	#print "new=$c\n";
	$fuzz_method = "Change all to lower case";
	Run();
}

#
# Insert strings from dictionary before each char
#
sub InsertDictionary
{
	print "\nInsertDictionary\n\n";

	$c =$o;

	for ($i = 0; $i <= length($c); $i++) {
		print "insert before pos=$i\n";
		for ($j = 0; $j <= $#dictionary; $j++) {
			$c = substr($c, 0, $i) . $dictionary[$j] . substr($c, $i);
			#print "new=$c\n";
			$fuzz_method = "Insert $dictionary[$j] before $i";
			Run();
			$c = $o;
		}
	}
}

#
# Run john
#
sub Run
{
	print "Trying format $f, hash $c\n";
	open(PW, "> $pwfile") || die;
	print PW "$c\n";
	close(PW);

	open(JOHN, "| $john_path --skip-self-tests --nolog --encoding=raw --stdin --session=$session --pot=$pot --format=$f $pwfile") || die;
	print JOHN "wrong password " x10 . "one\n";
	print JOHN "two wrongs\n";
	print JOHN "wrong password three\n";
	close(JOHN);

	die if ($? == 256 || $? == 2 || $? == 15); # exit(1) or INT or TERM

	if ($? != 0) {
		open(LOG, ">> fuzz-err.log") || die;
		print LOG "fuzz method=$fuzz_method\n";
		print LOG "$f $c $?\n";
		close(LOG);

		open(SAMPLE, "> fuzz-sample-$f-$cpu-$seq") || die;
		print SAMPLE "$c\n";
		close(SAMPLE);
		$seq++;
	}
}
