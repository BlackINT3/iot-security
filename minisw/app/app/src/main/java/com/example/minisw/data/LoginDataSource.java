package com.example.minisw.data;

import com.example.minisw.data.model.LoggedInUser;
import java.io.IOException;
import java.util.concurrent.FutureTask;

/**
 * Class that handles authentication w/ login credentials and retrieves user information.
 */
public class LoginDataSource {
    public Result<LoggedInUser> login(String username, String password) {

        try {
            String res = GlobalData.AuthGet(GlobalData.authUrl, username, password);
            if (!res.contains("err")) {
                LoggedInUser fakeUser = new LoggedInUser(username, password);
                return new Result.Success<>(fakeUser);
            }
            return new Result.Error(new Exception("Password error"));
        } catch (Exception e) {
            return new Result.Error(new IOException("Error logging in", e));
        }
    }

    public void logout() {
        // TODO: revoke authentication
    }
}
