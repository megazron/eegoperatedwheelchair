# EEG Signal Processing Template
# Based on original code by Backyard Brains, https://backyardbrains.com/
# Modified as a template by MegaZroN, portfolio https://megazron.com
# This template processes EEG data from a serial port, applies filters, extracts features,
# and uses a pre-trained model for classification.

import serial
import numpy as np
from scipy import signal
import pandas as pd
import time
import pickle
from collections import deque

# Suppress sklearn warnings
import warnings
warnings.filterwarnings("ignore", category=UserWarning)

def setup_filters(sampling_rate):
    """
    Set up notch and bandpass filters for EEG signal processing.
    Modify filter parameters (e.g., notch frequency, bandpass range) as needed.
    """
    b_notch, a_notch = signal.iirnotch(50.0 / (0.5 * sampling_rate), 30.0)  # Notch filter for 50 Hz noise
    b_bandpass, a_bandpass = signal.butter(4, [0.5 / (0.5 * sampling_rate), 30.0 / (0.5 * sampling_rate)], 'band')  # Bandpass filter
    return b_notch, a_notch, b_bandpass, a_bandpass

def process_eeg_data(data, b_notch, a_notch, b_bandpass, a_bandpass):
    """
    Apply notch and bandpass filters to the EEG data.
    Modify filter application or add additional processing steps as needed.
    """
    data = signal.filtfilt(b_notch, a_notch, data)
    data = signal.filtfilt(b_bandpass, a_bandpass, data)
    return data

def calculate_psd_features(segment, sampling_rate):
    """
    Calculate power spectral density (PSD) features for EEG frequency bands.
    Adjust frequency bands or add new features as needed.
    """
    f, psd_values = signal.welch(segment, fs=sampling_rate, nperseg=len(segment))
    bands = {'alpha': (8, 13), 'beta': (14, 30), 'theta': (4, 7), 'delta': (0.5, 3)}
    features = {}
    for band, (low, high) in bands.items():
        idx = np.where((f >= low) & (f <= high))
        features[f'E_{band}'] = np.sum(psd_values[idx])
    features['alpha_beta_ratio'] = features['E_alpha'] / features['E_beta'] if features['E_beta'] > 0 else 0
    return features

def calculate_additional_features(segment, sampling_rate):
    """
    Calculate additional spectral features such as peak frequency, centroid, and slope.
    Modify or extend features as needed.
    """
    f, psd = signal.welch(segment, fs=sampling_rate, nperseg=len(segment))
    peak_frequency = f[np.argmax(psd)]
    spectral_centroid = np.sum(f * psd) / np.sum(psd)
    log_f = np.log(f[1:])
    log_psd = np.log(psd[1:])
    spectral_slope = np.polyfit(log_f, log_psd, 1)[0]
    return {'peak_frequency': peak_frequency, 'spectral_centroid': spectral_centroid, 'spectral_slope': spectral_slope}

def load_model_and_scaler():
    """
    Load pre-trained machine learning model and scaler.
    Update file paths to match your model and scaler files.
    """
    with open('model.pkl', 'rb') as f:
        clf = pickle.load(f)
    with open('scaler.pkl', 'rb') as f:
        scaler = pickle.load(f)
    return clf, scaler

def main():
    """
    Main function to read EEG data from serial port, process it, and classify using a pre-trained model.
    Modify serial port, baud rate, buffer size, and classification actions as needed.
    """
    ser = serial.Serial('COM5', 115200, timeout=1)  # Update COM port as needed
    clf, scaler = load_model_and_scaler()
    b_notch, a_notch, b_bandpass, a_bandpass = setup_filters(512)  # Update sampling rate if needed
    buffer = deque(maxlen=512)  # Buffer size matches sampling rate

    while True:
        try:
            raw_data = ser.readline().decode('latin-1').strip()
            if raw_data:
                eeg_value = float(raw_data)
                buffer.append(eeg_value)

                if len(buffer) == 512:  # Process when buffer is full
                    buffer_array = np.array(buffer)
                    processed_data = process_eeg_data(buffer_array, b_notch, a_notch, b_bandpass, a_bandpass)
                    psd_features = calculate_psd_features(processed_data, 512)
                    additional_features = calculate_additional_features(processed_data, 512)
                    features = {**psd_features, **additional_features}

                    df = pd.DataFrame([features])
                    X_scaled = scaler.transform(df)
                    prediction = clf.predict(X_scaled)
                    print(f"Predicted Class: {prediction}")

                    # Customize actions based on prediction
                    if prediction == 0:
                        print("Stop")
                    elif prediction == 1 or prediction == 2:
                        print("Left")
                    elif prediction == 3:
                        print("Front")
                    buffer.clear()

        except Exception as e:
            print(f'Error: {e}')
            continue

if __name__ == '__main__':
    main()