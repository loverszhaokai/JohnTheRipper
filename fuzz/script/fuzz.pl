#!/usr/bin/perl -w

use Errno;

# Processes per logical CPU
$factor = 4;

$workdir = '/dev/shm/fuzz';
$pwfile = $workdir . '/pw';
$session = $workdir . '/s';
$pot = $workdir . '/pot';

die unless (mkdir($workdir, 0700) || $!{EEXIST});

$ENV{'OMP_NUM_THREADS'} = '1';
setpriority(PRIO_PROCESS, 0, 19);

sub try
{
	print "Trying format $f, hash $c\n";

	open(PW, "> $pwfile") || die;
	print PW "$c\n";
	close(PW);

	open(JOHN, "| ./john --skip-self-tests --nolog --encoding=raw --stdin --session=$session --pot=$pot --format=$f $pwfile") || die;
	print JOHN "wrong password " x10 . "one\n";
	print JOHN "two wrongs\n";
	print JOHN "wrong password three\n";
	close(JOHN);

	die if ($? == 256 || $? == 2 || $? == 15); # exit(1) or INT or TERM

	if ($? != 0) {
		open(LOG, ">> fuzz-err.log") || die;
		print LOG "$f $c $?\n";
		close(LOG);

		open(SAMPLE, "> fuzz-sample-$f-$cpu-$seq") || die;
		print SAMPLE "$c\n";
		close(SAMPLE);
		$seq++;
	}
}

open(TESTS, './john --list=format-tests --format=cpu | shuf |') || die;
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

	try;

	for ($i = 0; $i < length($c); $i++) {
		vec($c, $i, 8) = ord('9');
		next if ($c eq $o);
		try;
		$c = $o;
	}

	for ($i = 0; $i < length($c); $i++) {
		vec($c, $i, 8) = ord('$');
		next if ($c eq $o);
		try;
		$c = $o;
	}

	for ($i = 0; $i < length($c) - 1; $i++) {
		my $x = vec($c, $i, 8);
		vec($c, $i, 8) = vec($c, $i + 1, 8);
		vec($c, $i + 1, 8) = $x;
		next if ($c eq $o);
		try;
		$c = $o;
	}

	$j = 1;
	for ($i = 0; $i < 5; $i++) {
		$c = $o . chr(vec($o, length($o) - 1, 8)) x $j;
		try;
		$j *= ($j + 1);
	}
}
