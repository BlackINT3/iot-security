package com.example.minisw;

import android.os.Bundle;

import com.example.minisw.data.GlobalData;
import com.example.minisw.data.Result;
import com.google.android.material.floatingactionbutton.FloatingActionButton;
import com.google.android.material.snackbar.Snackbar;

import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.widget.Toolbar;

import android.view.View;
import android.widget.Button;
import android.widget.Toast;

public class MainActivity extends AppCompatActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        Toolbar toolbar = findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);

        final String user = getIntent().getStringExtra("user");
        final String passwd = getIntent().getStringExtra("passwd");

        Button openButton = findViewById(R.id.button);
        openButton.setOnClickListener(new View.OnClickListener(){
            @Override
            public void onClick(View view) {
                String url = GlobalData.authUrl + "?pc=1&act=1";
                String res = GlobalData.AuthGet(url, user, passwd);
                String msg;
                if (!res.contains("err")) {
                    msg = "返回："+res;
                } else {
                    msg = "请求错误";
                }
                Toast.makeText(getApplicationContext(), msg, Toast.LENGTH_SHORT).show();
            }
        });
        Button closeButton = findViewById(R.id.button2);
        closeButton.setOnClickListener(new View.OnClickListener(){
            @Override
            public void onClick(View view) {
                String url = GlobalData.authUrl + "?pc=1&act=0";
                String res = GlobalData.AuthGet(url, user, passwd);
                String msg;
                if (!res.contains("err")) {
                    msg = "返回："+res;
                } else {
                    msg = "请求错误";
                }
                Toast.makeText(getApplicationContext(), msg, Toast.LENGTH_SHORT).show();
            }
        });
        Button statButton = findViewById(R.id.button4);
        statButton.setOnClickListener(new View.OnClickListener(){
            @Override
            public void onClick(View view) {
                String url = GlobalData.authUrl;
                String res = GlobalData.AuthGet(url, user, passwd);
                String msg;
                if (!res.contains("err")) {
                    msg = "返回："+res;
                } else {
                    msg = "请求错误";
                }
                Toast.makeText(getApplicationContext(), msg, Toast.LENGTH_SHORT).show();
            }
        });
        Button exitButton = findViewById(R.id.button3);
        exitButton.setOnClickListener(new View.OnClickListener(){
            @Override
            public void onClick(View view) {
                System.exit(0);
            }
        });

        FloatingActionButton fab = findViewById(R.id.fab);
        fab.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                Snackbar.make(view, "Author: 深山修行之人 blackint3@gmail.com", Snackbar.LENGTH_LONG)
                        .setAction("Action", null).show();
            }
        });
    }

}
