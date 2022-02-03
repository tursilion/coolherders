//-----------------------------------------------------------------------------
// <copyright file="StoryModeSlateScreen.cs" company="HarmlessLion">
//  Copyright (C) HarmlessLion. All rights reserved.
// </copyright>
//-----------------------------------------------------------------------------

namespace CoolHerders.Screens
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Text;
    using CoolHerders.Housekeeping;
    using CoolHerders.Properties;
    using Microsoft.Xna.Framework;
    using Microsoft.Xna.Framework.Content;
    using Microsoft.Xna.Framework.Graphics;
    using Microsoft.Xna.Framework.Input;

    internal class StoryModeSlateScreen : GameScreen, IDisposable
    {
        #region Fields

        /// <summary>
        /// This tracks our temporary content manager
        /// </summary>
        private ContentManager content;

        /// <summary>
        /// This holds the big font for the picture caption
        /// </summary>
        private SpriteFont bigFont;

        /// <summary>
        /// This tracks the texture we will use to display the story mode picture
        /// </summary>
        private Texture2D pictureTexture;

        /// <summary>
        /// A temporary background texture
        /// </summary>
        private Texture2D gradientTexture;

        /// <summary>
        /// A texture used to represent button A.
        /// </summary>
        private Texture2D buttonATexture;

        /// <summary>
        /// The caption to display beneath the picture
        /// </summary>
        private List<string> pictureCaption;

        private int pictureCaptionLinesToScroll;

        private int pictureCaptionLinesToScrollNext;

        private float pictureCaptionDistance;

        private Vector2 pictureCaptionOrigin;

        /// <summary>
        /// The position of the first line of the picture caption
        /// </summary>
        private Vector2 pictureCaptionPositon;

        /// <summary>
        /// The unparsed picture caption
        /// </summary>
        private string pictureCaptionString;

        /// <summary>
        /// The picture number to display
        /// </summary>
        private int pictureNumber;

        /// <summary>
        /// Should we automatically advance to the next frame
        /// </summary>
        private bool autoAdvance;

        /// <summary>
        /// How long have we waited for the frame to advance
        /// </summary>
        private double autoAdvanceTimer;

        #endregion

        #region Initialization

        /// <summary>
        /// Initializes a new instance of the StoryModeSlateScreen class
        /// </summary>
        public StoryModeSlateScreen(int pictureFileNumber, string pictureCaption)
        {
            TransitionOnTime = TimeSpan.FromSeconds(0.5);
            TransitionOffTime = TimeSpan.FromSeconds(0.5);
            this.pictureNumber = pictureFileNumber;
            this.pictureCaptionString = pictureCaption;
            this.pictureCaptionLinesToScroll = 0;
            this.pictureCaption = new List<string>();
            this.autoAdvance = false;
        }

        /// <summary>
        /// Loads graphics content for this screen. The slate texture is quite
        /// big, so we use our own local ContentManager to load it. This allows us
        /// to unload before going finto the game itself, wheras if we
        /// used the shared ContentManager provided by the Game class, the content
        /// would remain loaded forever.
        /// </summary>
        public override void LoadContent()
        {
            if (this.content == null)
            {
                this.content = new ContentManager(ScreenManager.Game.Services, "Content");
            }

            LoadAPicture();
            this.gradientTexture = content.Load<Texture2D>("MiscGraphics\\gradient");
            this.bigFont = this.content.Load<SpriteFont>("TimesNewRoman");
            this.buttonATexture = content.Load<Texture2D>("Buttons - Small\\small_face_a");

            this.pictureCaptionPositon = new Vector2(ScreenManager.GraphicsDevice.Viewport.Width / 2, 560.0f);
            this.pictureCaptionLinesToScrollNext = FormatPictureCaption();
        }

        private void LoadAPicture()
        {
            try
            {
                this.pictureTexture = this.content.Load<Texture2D>(string.Format("Story\\StoryArt\\{0:D4}", pictureNumber));
            }
            catch (ContentLoadException)
            {
                this.pictureTexture = new Texture2D(ScreenManager.GraphicsDevice, 720, 540);
                Color[] fillColors = new Color[720 * 540];
                for (int counter = 0; counter < fillColors.Length; counter++)
                {
                    fillColors[counter] = Color.CornflowerBlue;
                }
                this.pictureTexture.SetData(fillColors);
            }
        }

        private int FormatPictureCaption()
        {
            int retval;
            int captionSplitPoint = pictureCaptionString.IndexOf("\\n");
            int beginningPoint = 0;
            if ((this.pictureCaptionString.Length > 0) && (this.pictureCaptionString[0] == '+'))
            {
                beginningPoint = 1;
                this.autoAdvance = true;
            }
            if (this.pictureCaptionString.Length == 0)
            {
                this.autoAdvance = true;
            }
            if (captionSplitPoint == -1)
            {
                float stringWidth = this.bigFont.MeasureString(this.pictureCaptionString).X;
                if (stringWidth / 2 > ScreenManager.GraphicsDevice.Viewport.TitleSafeArea.Width)
                {
                    captionSplitPoint = this.pictureCaptionString.Length / 2;
                    while ((captionSplitPoint > 0) && (this.pictureCaptionString[captionSplitPoint] != ' '))
                    {
                        captionSplitPoint--;
                    }
                    this.pictureCaption.Add(this.pictureCaptionString.Substring(beginningPoint, captionSplitPoint));
                    this.pictureCaption.Add(this.pictureCaptionString.Substring(captionSplitPoint + 1, (this.pictureCaptionString.Length - captionSplitPoint) - 1));
                    retval = 2;
                }
                else
                {
                    this.pictureCaption.Add(this.pictureCaptionString.Substring(beginningPoint, this.pictureCaptionString.Length - beginningPoint));
                    retval = 1;
                }
            }
            else
            {
                this.pictureCaption.Add(this.pictureCaptionString.Substring(beginningPoint, captionSplitPoint));
                this.pictureCaption.Add(this.pictureCaptionString.Substring(captionSplitPoint + 2, (this.pictureCaptionString.Length - captionSplitPoint) - 2));
                retval = 2;
            }

            int longestLine = 0;
            int longestLineLength = 0;
            for (int counter = 0; counter < this.pictureCaption.Count; counter++)
            {
                if (this.pictureCaption[counter].Length > longestLineLength)
                {
                    longestLineLength = this.pictureCaption[counter].Length;
                    longestLine = counter;
                }
            }

            this.pictureCaptionOrigin = this.bigFont.MeasureString(this.pictureCaption[longestLine]) / 2;

            return retval;
        }

        public void ShowNextPicture(int pictureFileNumber, string pictureCaption)
        {
            this.pictureNumber = pictureFileNumber;
            this.pictureCaptionString = pictureCaption;
            this.pictureCaptionLinesToScroll = this.pictureCaptionLinesToScrollNext;
            this.pictureCaptionDistance = 0;
            this.autoAdvance = false;
        }

        /// <summary>
        /// Unloads graphics content for this screen.
        /// </summary>
        public override void UnloadContent()
        {
            this.content.Unload();
        }

        #endregion

        /// <summary>
        /// Responds to user input, proceeding to the next screen
        /// </summary>
        /// <param name="input">The input state currently present in the system</param>
        public override void HandleInput(InputState input)
        {
            if ((this.ScreenState == ScreenState.Active) && (this.pictureCaptionLinesToScroll == 0))
            {
                if ((input.IsNewButtonPress(Buttons.A)) && (!this.autoAdvance))
                {
                    GameInformation.Instance.StoryModeSequencer.RunNextSequence(false, false);
                } else if (input.IsNewButtonPress(Buttons.Start))
                {
                    MessageBoxScreen confirmSkipMessageBox =
                                            new MessageBoxScreen(Resources.SkipStoryMode);

                    confirmSkipMessageBox.Accepted +=new EventHandler<EventArgs>(confirmSkipMessageBox_Accepted);

                    ScreenManager.AddScreen(confirmSkipMessageBox);
                }
            }
        }

        void confirmSkipMessageBox_Accepted(object sender, EventArgs e)
        {
            GameInformation.Instance.StoryModeSequencer.RunNextSequence(true, false);
        }

        #region Update and Draw

        /// <summary>
        /// Shows the title screen and waits for the player to press the 'start' button to assign the primary controller
        /// </summary>
        /// <param name="gameTime">GameTime class indicating the progression of time in the game</param>
        /// <param name="otherScreenHasFocus">Boolean set to true if some other screen has focus</param>
        /// <param name="coveredByOtherScreen">Boolean set to true if some other screen covers us</param>
        public override void Update(GameTime gameTime, bool otherScreenHasFocus, bool coveredByOtherScreen)
        {
            base.Update(gameTime, otherScreenHasFocus, coveredByOtherScreen);
            if (this.pictureCaptionLinesToScroll > 0)
            {
                this.autoAdvanceTimer = 0;
                float lineSpacing = (this.bigFont.LineSpacing / 2) + 10;
                float distanceMoved = (float)(gameTime.ElapsedGameTime.TotalSeconds * lineSpacing);
                this.pictureCaptionPositon.Y -= distanceMoved;
                this.pictureCaptionDistance += distanceMoved;
                if (this.pictureCaptionDistance >= lineSpacing)
                {
                    this.pictureCaptionLinesToScroll--;
                    this.pictureCaptionDistance = 0;
                    this.pictureCaption.RemoveAt(0);
                    this.pictureCaptionPositon.Y = (float)Math.Round(this.pictureCaptionPositon.Y) + lineSpacing;
                    if (this.pictureCaptionLinesToScroll == 0)
                    {
                        this.pictureCaptionPositon.Y = 560.0f;
                        this.pictureCaptionLinesToScrollNext = FormatPictureCaption();
                        LoadAPicture();
                        Debug.Assert(this.pictureCaptionLinesToScrollNext != 0);
                    }
                }
            }
            else
            {
                if (this.autoAdvance)
                {
                    this.autoAdvanceTimer += gameTime.ElapsedGameTime.TotalSeconds;
                    if (this.autoAdvanceTimer > 2)
                    {
                        this.autoAdvance = false;
                        GameInformation.Instance.StoryModeSequencer.RunNextSequence(false, false);
                    }
                }
            }
        }

        /// <summary>
        /// Draws the background screen.
        /// </summary>
        /// <param name="gameTime">GameTime class indicating the progression of time in the game</param>
        public override void Draw(GameTime gameTime)
        {
            SpriteBatch spriteBatch = ScreenManager.SpriteBatch;

            // Create a rectangle that is centered horizontally, and at the top of the screen vertically
            Rectangle storyPictureDisplayArea = new Rectangle((1280 - 720) / 2, 0, 720, 540);

            Rectangle fullScreenArea = new Rectangle(0, 0, 1280, 720);

            byte fade = TransitionAlpha;

            // XNA4.0
            //spriteBatch.Begin(SpriteBlendMode.AlphaBlend);
            spriteBatch.Begin();

            spriteBatch.Draw(this.gradientTexture, fullScreenArea, new Color(0, 0, 0));

            Vector2 fontStart = this.pictureCaptionPositon;

            if (this.ScreenState == ScreenState.TransitionOff)
            {
                fontStart.Y -= this.TransitionPosition * 100;
            }

            foreach (string captionLine in this.pictureCaption)
            {
                spriteBatch.DrawString(this.bigFont, captionLine, fontStart, new Color(fade, fade, fade), 0, pictureCaptionOrigin, 0.5f, SpriteEffects.None, 0);
                fontStart.Y += this.bigFont.LineSpacing / 2 + 10;
            }

            if ((this.ScreenState == ScreenState.Active) && (this.pictureCaptionLinesToScroll == 0) && (!this.autoAdvance))
            {
                Vector2 buttonLocation = new Vector2(ScreenManager.Game.GraphicsDevice.Viewport.TitleSafeArea.Right - this.buttonATexture.Width,
                    ScreenManager.Game.GraphicsDevice.Viewport.TitleSafeArea.Bottom - this.buttonATexture.Height);
                spriteBatch.Draw(this.buttonATexture, buttonLocation, Color.White);
            }

            Rectangle pictureScreenArea = new Rectangle(0, 0, 1280, 540);
            spriteBatch.Draw(this.gradientTexture, pictureScreenArea, new Color(0, 0, 0));
            spriteBatch.Draw(this.pictureTexture, storyPictureDisplayArea, new Color(fade, fade, fade));

            spriteBatch.End();
        }

        #endregion

        #region IDisposable Members

        /// <summary>
        /// This is the public dispose method that disposes managed and any unmanaged resources
        /// </summary>
        public void Dispose()
        {
            this.Dispose(true);
            GC.SuppressFinalize(this);
        }

        /// <summary>
        /// This is the internal dispose method that disposes the resources it has been instructed to
        /// </summary>
        /// <param name="disposing">Set to true if it is ok to dispose any managed resources</param>
        private void Dispose(bool disposing)
        {
            if (disposing)
            {
                this.content.Dispose();
                this.pictureTexture.Dispose();
            }
        }
        #endregion
    }
}
