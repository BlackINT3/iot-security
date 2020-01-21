package com.example.minisw.data;

import java.io.IOException;
import java.util.concurrent.Callable;
import java.util.concurrent.FutureTask;
import java.util.concurrent.TimeUnit;

import okhttp3.Authenticator;
import okhttp3.Credentials;
import okhttp3.OkHttpClient;
import okhttp3.Request;
import okhttp3.Response;
import okhttp3.Route;

public class GlobalData {
    //服务端地址
    public static String authUrl = "http://www.aaabbbccc.com:30000/minisw/";

    public static OkHttpClient buildBasicAuthClient(final String name, final String password) {
        return new OkHttpClient.Builder().authenticator(new Authenticator() {
            private int responseCount(Response response) {
                int result = 0;
                while ((response = response.priorResponse()) != null) {
                    result++;
                }
                return result;
            }
            @Override
            public Request authenticate(Route route, Response response) throws IOException {
                if (responseCount(response) >= 1) {
                    System.out.println("认证失败，返回null");
                    return null;
                }
                String credential = Credentials.basic(name, password);
                return response.request().newBuilder().header("Authorization", credential).build();
            }
        }).connectTimeout(2, TimeUnit.SECONDS).readTimeout(2, TimeUnit.SECONDS).build();
    }

    public static class AuthRequest implements Callable<String>{
        private String username;
        private String password;
        private String url;

        public AuthRequest(String user, String passwd, String url) {
            this.username = user;
            this.password = passwd;
            this.url = url;
        }

        public String call() throws Exception {
            OkHttpClient client = GlobalData.buildBasicAuthClient(username, password);
            Request request = new Request.Builder()
                    .url(url)
                    .build();
            Response response = client.newCall(request).execute();
            if (!response.isSuccessful()) throw new IOException("Unexpected code " + response);
            return response.body().string();
        }
    };

    public static String AuthGet(String url, String username, String password) {
        try {
            FutureTask<String> ftask = new FutureTask<String>(new GlobalData.AuthRequest(username, password, url));
            Thread t = new Thread(ftask);
            t.start();
            t.join();
            String res = ftask.get();
            return res;
        } catch (Exception e) {
            return "err";
        }
    }
}
