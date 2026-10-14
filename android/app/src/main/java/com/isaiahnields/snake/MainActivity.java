package com.isaiahnields.snake;

import org.libsdl.app.SDLActivity;

import androidx.annotation.Keep;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.URL;
import java.nio.charset.StandardCharsets;

import javax.net.ssl.HttpsURLConnection;

public class MainActivity extends SDLActivity {
    // Called by the native authentication worker; Android supplies its trusted TLS certificates.
    @Keep
    public byte[] postCognito(byte[] operation, byte[] body) throws IOException {
        HttpsURLConnection connection = (HttpsURLConnection) new URL(
                "https://cognito-idp.us-west-2.amazonaws.com/").openConnection();
        try {
            connection.setConnectTimeout(5000);
            connection.setReadTimeout(10000);
            connection.setInstanceFollowRedirects(false);
            connection.setRequestMethod("POST");
            connection.setRequestProperty("Content-Type", "application/x-amz-json-1.1");
            connection.setRequestProperty("X-Amz-Target",
                    "AWSCognitoIdentityProviderService."
                            + new String(operation, StandardCharsets.UTF_8));
            connection.setDoOutput(true);
            connection.setFixedLengthStreamingMode(body.length);
            try (OutputStream output = connection.getOutputStream()) {
                output.write(body);
            }
            int status = connection.getResponseCode();
            ByteArrayOutputStream response = new ByteArrayOutputStream();
            response.write(Integer.toString(status).getBytes(StandardCharsets.UTF_8));
            InputStream stream = status >= 400 ? connection.getErrorStream()
                    : connection.getInputStream();
            if (stream == null) {
                throw new IOException("Missing Cognito response body");
            }
            try (InputStream input = stream) {
                byte[] buffer = new byte[4096];
                int count;
                while ((count = input.read(buffer)) != -1) {
                    if (response.size() + count > 1024 * 1024) {
                        throw new IOException("Cognito response exceeded the size limit");
                    }
                    response.write(buffer, 0, count);
                }
            }
            return response.toByteArray();
        } finally {
            connection.disconnect();
        }
    }

    @Override
    protected String[] getLibraries() {
        return new String[] {
                "SDL3",
                "main" // Must match the target name in CMakeLists.txt
        };
    }
}
