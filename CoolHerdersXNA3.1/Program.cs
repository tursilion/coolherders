//-----------------------------------------------------------------------------
// <copyright file="Program.cs" company="HarmlessLion">
//  Copyright (C) HarmlessLion. All rights reserved.
// </copyright>
//-----------------------------------------------------------------------------

namespace CoolHerders
{
    using System;
    using System.Diagnostics.CodeAnalysis;

    /// <summary>
    /// This class holds the main program run loop
    /// </summary>
    public static class Program
    {
        /// <summary>
        /// This is the main function for the game
        /// </summary>
        /// <param name="args">A string array containing the command line arguments</param>
        [SuppressMessage("Microsoft.Usage", "CA1801:ReviewUnusedParameters", Justification = "We don't use command line arguments", MessageId = "args")]
        public static void Main(string[] args)
        {
            using (CoolHerdersGame game = new CoolHerdersGame())
            {
                game.Run();
            }
        }
    }
}

