package omegacentauri.mobi.relay;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.SharedPreferences;
import android.os.AsyncTask;
import android.os.Build;
import android.os.Bundle;
import android.preference.PreferenceManager;
import android.text.Editable;
import android.text.InputType;
import android.text.TextWatcher;
import android.util.Log;
import android.view.KeyEvent;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;

import java.io.IOException;
import java.security.KeyManagementException;
import java.security.NoSuchAlgorithmException;
import java.util.Timer;
import java.util.TimerTask;

import javax.net.ssl.HostnameVerifier;
import javax.net.ssl.SSLContext;
import javax.net.ssl.SSLSession;
import javax.net.ssl.SSLSocketFactory;
import javax.net.ssl.TrustManager;
import javax.net.ssl.X509TrustManager;

import okhttp3.Call;
import okhttp3.Callback;
import okhttp3.MultipartBody;
import okhttp3.OkHttpClient;
import okhttp3.Request;
import okhttp3.RequestBody;
import okhttp3.Response;

public class MainActivity extends Activity implements Callback {
    private EditText code;
    private Button buttonGo;
    private TextView status;
    private OkHttpClient client;

    static private OkHttpClient InsecureOkHttpClient() throws NoSuchAlgorithmException, KeyManagementException {
        final TrustManager[] trustAllCerts = new TrustManager[] {
                new X509TrustManager() {
                    @Override
                    public void checkClientTrusted(java.security.cert.X509Certificate[] chain, String authType) {
                    }

                    @Override
                    public void checkServerTrusted(java.security.cert.X509Certificate[] chain, String authType) {
                    }

                    @Override
                    public java.security.cert.X509Certificate[] getAcceptedIssuers() {
                        return new java.security.cert.X509Certificate[]{};
                    }
                }
        };

        // Install the all-trusting trust manager
        final SSLContext sslContext = SSLContext.getInstance("SSL");
        sslContext.init(null, trustAllCerts, new java.security.SecureRandom());
        // Create an ssl socket factory with our all-trusting manager
        final SSLSocketFactory sslSocketFactory = sslContext.getSocketFactory();

        OkHttpClient.Builder builder = new OkHttpClient.Builder();
        builder.sslSocketFactory(sslSocketFactory, (X509TrustManager)trustAllCerts[0]);
        builder.hostnameVerifier(new HostnameVerifier() {
            @Override
            public boolean verify(String hostname, SSLSession session) {
                return true;
            }
        });
        return builder.build();
    }

    private static final int CODE_TIMEOUT = 60000;
    private Timer clearTimer;
    private TimerTask clearTask;
    private SharedPreferences options;
    private boolean busy = false;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        try {
            client = InsecureOkHttpClient();
        } catch (NoSuchAlgorithmException e) {
            client = null;
        } catch (KeyManagementException e) {
            e.printStackTrace();
        }

        setContentView(R.layout.main);
        status = (TextView)findViewById(R.id.status);
        code = (EditText)findViewById(R.id.code);
        options = PreferenceManager.getDefaultSharedPreferences(this);

        clearTimer = null;

        code.addTextChangedListener(new TextWatcher() {
            @Override
            public void beforeTextChanged(CharSequence charSequence, int i, int i1, int i2) {

            }

            @Override
            public void onTextChanged(CharSequence charSequence, int i, int i1, int i2) {
                updateStatus();
                resetTimer();
            }

            @Override
            public void afterTextChanged(Editable editable) {

            }
        });
        code.setOnKeyListener(new View.OnKeyListener() {
            @Override
            public boolean onKey(View view, int keyCode, KeyEvent keyEvent) {
                if ((keyEvent.getAction() == KeyEvent.ACTION_DOWN) &&
                        (keyCode == KeyEvent.KEYCODE_ENTER)) {
                    makePost();
                    return true;
                }
                return false;
            }
        });
        buttonGo = (Button)findViewById(R.id.go);

        resetTimer();
    }

    private void updateStatus() {
        final int state;
        if (code.getText().length() > 0 && getURL() != "" && !busy)
            state = View.VISIBLE;
        else
            state = View.INVISIBLE;
        if (buttonGo.getVisibility() != state) {
            runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    buttonGo.setVisibility(state);
                }
            });
        }
    }

    public void goButton(View view) {
        makePost();
    }

    public void optionsButton(View view) {
        AlertDialog.Builder b = new AlertDialog.Builder(this);
        final EditText urlField = new EditText(this);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.CUPCAKE) {
            urlField.setInputType(InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_VARIATION_URI);
        }
        String url = getURL();
        if (url == "")
            url = "http://000.000.000.000/RELAY";
        url = ""; // TODO: be smarter
        urlField.setText(url);
        b.setView(urlField);
        b.setPositiveButton("OK", new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialogInterface, int i) {
                String text = String.valueOf(urlField.getText());
                if (text.length()>0) {
//                    Log.v("relay", "setting to "+text);
                    SharedPreferences.Editor ed = options.edit();
                    ed.putString("URL", text);
                    ed.commit();
                    updateStatus();
                }
            }
        });
        b.setNegativeButton("Cancel", new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialogInterface, int i) {

            }
        });
        b.create().show();
    }

    public void resetTimer() {
        if (clearTimer != null) {
            try {
                clearTimer.cancel();
            }
            catch(Exception e) {}
            clearTimer = null;
        }
        clearTimer = new Timer();
        clearTask = new TimerTask() {
            @Override
            public void run() {
                runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        code.setText("");
                    }
                });
            }
        };
        clearTimer.schedule(clearTask, CODE_TIMEOUT);
    }

    @Override
    protected void onResume() {
        super.onResume();

        status.setText("Ready...");
        code.setText("");
        updateStatus();
        code.requestFocus();
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.CUPCAKE) {
            code.setImeActionLabel("Go!", KeyEvent.KEYCODE_ENTER);
        }

        //InputMethodManager imm = (InputMethodManager) getSystemService(INPUT_METHOD_SERVICE);
        //imm.showSoftInput(code, InputMethodManager.SHOW_FORCED);
        //imm.toggleSoftInput(InputMethodManager.SHOW_FORCED,InputMethodManager.HIDE_IMPLICIT_ONLY);
    }

    private String getURL() {
        String url = options.getString("URL", "");
        if (url.equals(""))
            return url;
        if (! url.startsWith("http://") && ! url.startsWith("https://"))
            url = "http://" + url;
        /*if (! url.contains("/RELAY"))
            url = url + "/RELAY"; */
        return url;
    }

    private boolean makePost() {
        if (busy || code.getText().length() == 0) {
            return false;
        }

        resetTimer();

        busy = true;
        updateStatus();
        status.setText("Working...");

        RequestBody requestBody = new MultipartBody.Builder()
                .setType(MultipartBody.FORM)
                .addFormDataPart("code", String.valueOf(code.getText()))
                .build();

        Request request = new Request.Builder()
                .url(getURL())
                .post(requestBody)
                .build();

        if (client != null)
            client.newCall(request).enqueue(this);
        else
            onFailure(null, null);
        return true;
    }

    @Override
    public void onFailure(Call call, IOException e) {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
//                Log.v("relay", "exception "+e);
                status.setText("Connection failed!");
            }
        });
        busy = false;
        updateStatus();
    }

    @Override
    public void onResponse(Call call, Response response) throws IOException {
        final String out = response.body().string().toLowerCase();
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                if (out.contains("sesame opening"))
                    status.setText("Triggered!");
                else if (out.contains("too many tries"))
                    status.setText("Failed too many times. Retry later.");
                else if (out.contains("incorrect entry")) {
                    status.setText("Wrong code");
                    code.setText("");
                }
                else {
                    status.setText("Unknown response.");
                    Log.e("relay", out);
                }
                busy = false;
                updateStatus();
            }
        });
    }

    private class PostTask extends AsyncTask<String, String, String> {
        @Override
        protected String doInBackground(String... data) {
            return "";
        }
    }

}
