"""
Flask Documentation:     https://flask.palletsprojects.com/
Jinja2 Documentation:    https://jinja.palletsprojects.com/
Werkzeug Documentation:  https://werkzeug.palletsprojects.com/
This file creates your application.
"""

# from crypt import methods
import site 

from app import app, Config,  mongo, Mqtt
from flask import escape, render_template, request, jsonify, send_file, redirect, make_response, send_from_directory 
from json import dumps, loads 
from werkzeug.utils import secure_filename
from datetime import datetime,timedelta, timezone
from os import getcwd
from os.path import join, exists
from time import time, ctime
from math import floor
 



#####################################
#   Routing for your application    #
#####################################


# 1. CREATE ROUTE FOR '/api/set/combination'
@app.route('/api/set/combination', methods=["POST"])
def set_combination():
    if request.method == "POST":
        try:
            passcode = request.form.get("passcode")

            #Must be integer and exactly 4 digits
            if passcode is None or len(passcode) != 4 or not passcode.isdigit():
                return jsonify({"status": "failed", "data": "failed"})

            result = mongo.setPasscode(passcode)

            if result:
                return jsonify({"status": "complete", "data": "complete"})

        except Exception as e:
            print(f"set_combination error: {str(e)}")

    return jsonify({"status": "failed", "data": "failed"})

# 2. CREATE ROUTE FOR '/api/check/combination'
@app.route('/api/check/combination', methods=["POST"])
def check_combination():
    if request.method == "POST":
        try:
            passcode = request.form.get("passcode")

            if passcode is None:
                return jsonify({"status": "failed", "data": "failed"})

            count = mongo.checkPasscode(passcode)

            if count > 0:
                return jsonify({"status": "complete", "data": "complete"})

        except Exception as e:
            print(f"check_combination error: {str(e)}")

    return jsonify({"status": "failed", "data": "failed"})

# 3. CREATE ROUTE FOR '/api/update'
@app.route('/api/update', methods=["POST"])
def update():
    if request.method == "POST":
        try:
            data = request.get_json(force=True)

            if data is None:
                return jsonify({"status": "failed", "data": "failed"})

            data["timestamp"] = floor(time())

            Mqtt.publish("620171573", dumps(data))
            result = mongo.update(data)

            if result:
                return jsonify({"status": "complete", "data": "complete"})

        except Exception as e:
            print(f"update error: {str(e)}")

    return jsonify({"status": "failed", "data": "failed"})

# 4. CREATE ROUTE FOR '/api/reserve/<start>/<end>'
@app.route('/api/reserve/<start>/<end>', methods=["GET"])
def get_reserve(start, end):
    if request.method == "GET":
        try:
            data = mongo.getReserve(start, end)

            if data is not None and len(data) > 0:
                return jsonify({"status": "found", "data": data})

        except Exception as e:
            print(f"get_reserve error: {str(e)}")

    return jsonify({"status": "failed", "data": 0})

# 5. CREATE ROUTE FOR '/api/avg/<start>/<end>'
@app.route('/api/avg/<start>/<end>', methods=["GET"])
def avg_reserve(start, end):
    if request.method == "GET":
        try:
            result = mongo.avgReserve(start, end)

            if result and len(result) > 0:
                avg = result[0]["average"]
                return jsonify({"status": "found", "data": avg})

        except Exception as e:
            print(f"avg_reserve error: {str(e)}")

    return jsonify({"status": "failed", "data": 0})


   






@app.route('/api/file/get/<filename>', methods=['GET']) 
def get_images(filename):   
    '''Returns requested file from uploads folder'''
   
    if request.method == "GET":
        directory   = join( getcwd(), Config.UPLOADS_FOLDER) 
        filePath    = join( getcwd(), Config.UPLOADS_FOLDER, filename) 

        # RETURN FILE IF IT EXISTS IN FOLDER
        if exists(filePath):        
            return send_from_directory(directory, filename)
        
        # FILE DOES NOT EXIST
        return jsonify({"status":"file not found"}), 404


@app.route('/api/file/upload',methods=["POST"])  
def upload():
    '''Saves a file to the uploads folder'''
    
    if request.method == "POST": 
        file     = request.files['file']
        filename = secure_filename(file.filename)
        file.save(join(getcwd(),Config.UPLOADS_FOLDER , filename))
        return jsonify({"status":"File upload successful", "filename":f"{filename}" })

 


###############################################################
# The functions below should be applicable to all Flask apps. #
###############################################################


@app.after_request
def add_header(response):
    """
    Add headers to both force latest IE rendering engine or Chrome Frame,
    and also tell the browser not to cache the rendered page. If we wanted
    to we could change max-age to 600 seconds which would be 10 minutes.
    """
    response.headers['X-UA-Compatible'] = 'IE=Edge,chrome=1'
    response.headers['Cache-Control'] = 'public, max-age=0'
    return response

@app.errorhandler(405)
def page_not_found(error):
    """Custom 404 page."""    
    return jsonify({"status": 404}), 404



