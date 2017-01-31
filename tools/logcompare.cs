using System.Collections.Generic;
using System.IO;
using System;

class LogCompare
{
	// takes 2 logs, and sees which instructions the first uses that the second doesn't
	public static void Main(string[] args)
	{
		HashSet<string> ops = new HashSet<string>();
		string[] f1, f2;
		f1 = File.ReadAllLines(args[0]);
		f2 = File.ReadAllLines(args[1]);
		
		foreach (var line in f1)
		{
			if (!ops.Contains(line))
				ops.Add(line);
		}
		
		foreach (var line in f2)
		{
			if (!ops.Contains(line))
				Console.WriteLine(line);
		}
	}
}