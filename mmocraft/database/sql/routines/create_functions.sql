CREATE FUNCTION dbo.GetPasswordHash(@password VARCHAR(64))
RETURNS BINARY(64)
AS
BEGIN
    RETURN HASHBYTES('SHA2_256', '$(PasswordSalt)' + SUBSTRING(@password, 0, LEN(@password) + 1));
END;
GO