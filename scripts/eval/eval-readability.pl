#!/usr/bin/perl
#-*-perl-*-

use FindBin qw($Bin);


# my $lcurve_log = shift(@ARGV);

my $testset_src = shift(@ARGV);
my $testset_ref = shift(@ARGV);

my @output = @ARGV;

die "cannot find $testset_src\n" unless (-e $testset_src);
die "cannot find $testset_ref\n" unless (-e $testset_ref);

my %bleu = ();
my %parity = ();

my $steps = 0;
foreach my $o (@output){
    $steps++;
    if ($o=~/[^0-9]([0-9]+).xml/){
	$steps=$1;
    }
    print STDERR "evaluate after $steps steps ...\n";

    my $bleu_out = `$Bin/mteval-v13.pl -s $testset_src -r $testset_ref -t $o`;
    if ($bleu_out=~/BLEU score = ([0-1].[0-9]+)\s/s){
	$bleu{$steps} = $1;
    }

    @{$parity{$steps}} = type_token_ratio($o);
}

foreach (sort {$a <=> $b} keys %bleu){
    print "$_\t$bleu{$_}\t";
    print join("\t",@{$parity{$_}});
    print "\n";
}


sub type_token_ratio{
    my $file=shift;
    my $doc=0;

    my %words = ();
    my $nrTokens = 0;
    my $nrTypes = 0;
    my $total = 0;

    open F,"<$file" || die "cannot read from $file\n";
    my @ret=();

    while(<F>){
	if (/\<\/doc\>/){
	    $doc++;
	    my $types = scalar keys %words;
	    if ($types){ push(@ret,$nrTokens / $types); }
	    else{ push(@ret,0); }
	    $nrTypes += scalar keys %words;
	    $total += $nrTokens;
	    %words = ();
	    $nrTokens = 0;
	}
	next unless (/\<seg/);
	s/<.*?>//g;
	@w = split(/\s+/);
	foreach (@w){ $words{$_}++; }
	$nrTokens += scalar @w;
    }

    $nrTypes += scalar keys %words;
    unshift(@ret,$total/$nrTypes);
    return @ret;
}
