//-----------------------------------------------------------------------------
// <copyright file="MazeMapWriter.cs" company="HarmlessLion">
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
    using Microsoft.Xna.Framework.Content.Pipeline.Processors;
    using Microsoft.Xna.Framework.Content.Pipeline.Serialization.Compiler;
    using Microsoft.Xna.Framework.Graphics;

    using TWrite = MazeMap;

    /// <summary>
    /// This writes the maze map object to an application specific binary form
    /// </summary>
    [ContentTypeWriter]
    public class MazeMapWriter : ContentTypeWriter<TWrite>
    {
        /// <summary>
        /// This function returns the object that will be used on the game side to read this data
        /// </summary>
        /// <param name="targetPlatform">The type of platform that the build is for</param>
        /// <returns>A string containing the object name for later use</returns>
        public override string GetRuntimeReader(TargetPlatform targetPlatform)
        {
            return "CoolHerders.MazeMapReader, CoolHerders";
        }

        /// <summary>
        /// Writes the MazeMap object to disk in XNB format
        /// </summary>
        /// <param name="output">A XNA managed object that knows how to write in XNB format</param>
        /// <param name="value">The object to be written in XNB format</param>
        protected override void Write(ContentWriter output, TWrite value)
        {
            output.Write(value.MazeXDimension);
            output.Write(value.MazeYDimension);
            foreach (MazeMap.MazeTile tile in value.MazeTiles)
            {
                output.Write(tile.ImageTileSet);
                output.Write(tile.ImageNumber);
                output.Write(tile.IsPassable);
                output.Write(tile.IsDestructable);
                output.Write(tile.Is3D);
                output.Write(tile.UnderneathTileSet);
                output.Write(tile.UnderneathImageNumber);
            }
        }
    }
}
