//-----------------------------------------------------------------------------
// <copyright file="MazeMapReader.cs" company="HarmlessLion">
//  Copyright (C) HarmlessLion. All rights reserved.
//  Copyright (C) Microsoft.  From XNA sample code.
// </copyright>
//-----------------------------------------------------------------------------

namespace CoolHerders
{
    using System;
    using System.Collections.Generic;
    using Microsoft.Xna.Framework;
    using Microsoft.Xna.Framework.Content;
    using Microsoft.Xna.Framework.Graphics;

    using TRead = MazeMap;

    /// <summary>
    /// This class reads in MazeMap objects from the XNB files
    /// </summary>
    public class MazeMapReader : ContentTypeReader<TRead>
    {
        /// <summary>
        /// This function reads a maze map in from the XNB file, creating a MazeMap object
        /// </summary>
        /// <param name="input">The ContentReader that is linked to the required XNB file</param>
        /// <param name="existingInstance">An existing instance, if any, that we should read into</param>
        /// <returns>A MazeMapImporter.MazeMap object</returns>
        protected override TRead Read(ContentReader input, TRead existingInstance)
        {
            int mazeXSize;
            int mazeYSize;

            mazeXSize = input.ReadInt32();
            mazeYSize = input.ReadInt32();
            MazeMap tempMazeMap = existingInstance;
            if (null == existingInstance)
            {
                tempMazeMap = new MazeMap(mazeXSize, mazeYSize);
            }

            int totalMazeTiles = mazeXSize * mazeYSize;
            for (int counter = 0; counter < totalMazeTiles; counter++)
            {
                int imageTileSet = input.ReadInt32();
                int imageNumber = input.ReadInt32();
                bool isPassable = input.ReadBoolean();
                bool isDestructable = input.ReadBoolean();
                bool is3D = input.ReadBoolean();
                int underTileSet = input.ReadInt32();
                int underTileNumber = input.ReadInt32();
                tempMazeMap.AddTile(new MazeMap.MazeTile(imageTileSet, imageNumber, isPassable, isDestructable, is3D, underTileSet, underTileNumber));
            }

            return tempMazeMap;
        }
    }
}
