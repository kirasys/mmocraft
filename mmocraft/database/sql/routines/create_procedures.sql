CREATE PROCEDURE dbo.PlayerLogin
(
    @Username VARCHAR(64),
    @Password VARCHAR(64),
    @UserID INT OUTPUT,
    @UserType INT OUTPUT,
    @PlayerGameData BINARY(64) OUTPUT
)
AS 
BEGIN
    SET NOCOUNT ON

    -- Default values
    SET @UserID = 0
    SET @UserType = 0
    SET @PlayerGameData = 0

    DECLARE @UserFound INT
    DECLARE @IsAdmin BIT

    SELECT @UserFound = COUNT(*) FROM player WHERE username = @Username;

    IF (@UserFound = 0)
    BEGIN
        SET @UserType = 1 -- GUEST
        RETURN
    END
    
    SELECT @UserID = id, @IsAdmin = is_admin FROM player WHERE username = @Username AND password = dbo.GetPasswordHash(@Password);

    IF (@UserID = 0)
    BEGIN
        SET @UserType = 0 -- INVALID
        RETURN
    END

    SET @UserType = IIF(@IsAdmin != 0, 3, 2) -- is_admin ? ADMIN : AUTHENTICATED_USER
    SELECT @PlayerGameData = gamedata FROM player_game_data WHERE player_id = @UserID;
    RETURN
END
GO