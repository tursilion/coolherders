//-----------------------------------------------------------------------------
// <copyright file="MazeMapImporter.cs" company="HarmlessLion">
//  Copyright (C) HarmlessLion. All rights reserved.
// </copyright>
//-----------------------------------------------------------------------------
namespace MazeMapImporter
{
    using System;
    using System.Collections.Generic;
    using Microsoft.Xna.Framework;
    using Microsoft.Xna.Framework.Content.Pipeline;
    using Microsoft.Xna.Framework.Content.Pipeline.Graphics;
    using Microsoft.Xna.Framework.Graphics;

    using TImport = MazeMapTextSource;

    /// <summary>
    /// <para>
    /// This class will be instantiated by the XNA Framework Content Pipeline
    /// to import a file from disk into the specified type, TImport.
    /// </para>
    /// <para>
    /// This should be part of a Content Pipeline Extension Library project.
    /// </para>
    /// </summary>
    [ContentImporter(".mazemap", DisplayName = "Cool Herders Maze Map Importer", DefaultProcessor = "MazeMapProcessor")]
    public class MazeMapImporter : ContentImporter<TImport>
    {
        /// <summary>
        /// This function reads the actual input maze map into a source object and links this to the processor for the next step
        /// </summary>
        /// <param name="filename">The filename in the content pipeline</param>
        /// <param name="context">The importer context</param>
        /// <returns>A MazeMapTextSource object holding the text that needs processing</returns>
        public override TImport Import(string filename, ContentImporterContext context)
        {
            string mazeMapSourceCode = System.IO.File.ReadAllText(filename);
            return new MazeMapTextSource(mazeMapSourceCode);
        }
    }
}
