//-----------------------------------------------------------------------------
// <copyright file="MazeMapProcessor.cs" company="HarmlessLion">
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
    using Microsoft.Xna.Framework.Graphics;

    using TInput = MazeMapTextSource;
    using TOutput = MazeMap;

    /// <summary>
    /// This class processes the memory source form of a Maze Map into the machine usable form
    /// </summary>
    [ContentProcessor(DisplayName = "MazeMapImporter.MazeMapProcessor")]
    public class MazeMapProcessor : ContentProcessor<TInput, TOutput>
    {
        /// <summary>
        /// Processes maze map source into the in memory object
        /// </summary>
        /// <param name="input">Maze map source to process</param>
        /// <param name="context">The content processor context</param>
        /// <returns>An object holding all tiles indicated for the maze map</returns>
        public override TOutput Process(TInput input, ContentProcessorContext context)
        {
            string[] mazeTextRows = input.SourceCode.Replace("\r", String.Empty).Split('\n');
            int rows = 0;
            foreach (string mazeLine in mazeTextRows)
            {
                if (String.Empty != mazeLine)
                {
                    rows++;
                }
            }

            // TODO: Understand what the last line is for
            rows--;

            int columns;

            if (rows > 0)
            {
                int rowLength = mazeTextRows[0].Length - 2;
                if ((rowLength % 3) != 0)
                {
                    throw new System.IO.InvalidDataException("Maze Map first row not divisible by 3");
                }
                else
                {
                    columns = rowLength / 3;
                }
            }
            else
            {
                throw new System.IO.InvalidDataException("Unable to locate any recognizable data in the maze map");
            }

            // We want to skip the first row
            rows--;

            MazeMap newMap = new MazeMap(columns, rows);

            for (int rowCounter = 1; rowCounter <= rows; rowCounter++)
            {
                for (int columnCounter = 0; columnCounter < columns; columnCounter++)
                {
                    string mazeCode = mazeTextRows[rowCounter].Substring((columnCounter * 3) + 1, 3);
                    string characterConversionArray = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";

                    int bankNumber = characterConversionArray.IndexOf(mazeCode[0]) - 52;
                    if ((bankNumber < 1) || (bankNumber > 3))
                    {
                        throw new System.IO.InvalidDataException("Bank number code invalid");
                    }

                    bool isPassable;
                    if ('.' == mazeCode[1])
                    {
                        isPassable = true;
                    }
                    else
                    {
                        isPassable = false;
                    }

                    bool is3D;
                    if ('/' == mazeCode[1])
                    {
                        is3D = true;
                    }
                    else
                    {
                        is3D = false;
                    }

                    int tileNumber = characterConversionArray.IndexOf(mazeCode[2]);

                    int underneathTileSet = 0;
                    int underneathTileNumber = 0;
                    bool isDestructable = false;
                    if (bankNumber == 3)
                    {
                        isDestructable = true;
                        underneathTileNumber = characterConversionArray.IndexOf(mazeCode[1]);
                        if (underneathTileNumber >= 52)
                        {
                            throw new System.IO.InvalidDataException("Illegal tile specified for underneath tile");
                        }

                        if (underneathTileNumber >= 26)
                        {
                            underneathTileNumber -= 26;
                            underneathTileSet = 2;
                        }
                        else
                        {
                            underneathTileSet = 1;
                        }
                    }

                    MazeMap.MazeTile newTile = new MazeMap.MazeTile(bankNumber, tileNumber, isPassable, isDestructable, is3D, underneathTileSet, underneathTileNumber);
                    
                    newMap.AddTile(newTile);
                }
            }

            return newMap;
        }
    }
}