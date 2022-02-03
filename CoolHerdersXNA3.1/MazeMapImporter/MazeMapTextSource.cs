//-----------------------------------------------------------------------------
// <copyright file="MazeMapTextSource.cs" company="HarmlessLion">
//  Copyright (C) HarmlessLion. All rights reserved.
// </copyright>
//-----------------------------------------------------------------------------
namespace MazeMapImporter
{
    using System;
    using System.Collections.Generic;
    using System.Text;
    using Microsoft.Xna.Framework.Content.Pipeline;

    /// <summary>
    /// This holds the text strings retreived from the input file for the maze definition
    /// </summary>
    public class MazeMapTextSource
    {
        /// <summary>
        /// This holds the exact string contained in the disk file
        /// </summary>
        private string sourceCode;

        /// <summary>
        /// Initializes a new instance of the MazeMapTextSource class.
        /// </summary>
        /// <param name="sourceCode">The maze definition string</param>
        public MazeMapTextSource(string sourceCode)
        {
            this.sourceCode = sourceCode;
        }

        /// <summary>
        /// Gets the maze source string
        /// </summary>
        public string SourceCode
        {
            get
            {
                return this.sourceCode;
            }
        }
    }
}
