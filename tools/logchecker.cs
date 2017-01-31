using System.Collections.Generic;
using System.IO;
using System;

public class LogChecker
{
	public static void Main(string[] args)
	{
		HashSet<byte> opcodes, cbOpcodes;
		List<string> lines = new List<string>();
		opcodes = new HashSet<byte>();
		cbOpcodes = new HashSet<byte>();
		byte[] bytes = File.ReadAllBytes("log.txt");
		
		for (int i = 0; i < bytes.Length; i++)
		{
			if (bytes[i] == 0xCB)
			{
				i++;
				cbOpcodes.Add(bytes[i]);
			}	
			else
			{
				opcodes.Add(bytes[i]);
			}
		}
		
		for (int i = 0; i < 0x100; i++)
		{
			if (!opcodes.Contains((byte)i))
			{
				lines.Add(string.Format("0x{0:X2}", i));
			}
			
			if (!cbOpcodes.Contains((byte)i))
			{
				lines.Add(string.Format("CB 0x{0:X2}", i));
			}
		}
		
		File.WriteAllLines(args[0], lines.ToArray());
	}
}